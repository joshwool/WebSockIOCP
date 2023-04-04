#include <thread.hpp>

using json = nlohmann::json;

char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char hexmap[] = "0123456789abcdef";

Thread::Thread(HANDLE iocPort) : m_handle(INVALID_HANDLE_VALUE), m_threadId(0) {
    m_handle = CreateThread( // Creates a thread
	  nullptr,
	  0,
	  Thread::IoWork,
	  iocPort,
	  0,
	  &m_threadId);

    if (m_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateThread() failed: " << GetLastError() << std::endl;
    }
}

Thread::~Thread() {
    Terminate();
}

DWORD WINAPI Thread::IoWork(LPVOID IoCPort) {
    auto iocpHandle = (HANDLE)IoCPort;

    while (true) {
		DWORD totalBytes; // Total bytes involved in completed operation.
 		IoContext *pIoContext; // Pointer to an IO context object;
		LPWSAOVERLAPPED overlapped; // Pointer to overlapped struct, passed with every IO operation.

        if (GetQueuedCompletionStatus( // Gets completion packets from completion queue
                iocpHandle,
                &totalBytes,
                (PULONG_PTR)&pIoContext, // Assigns completion key pointer to pIoContext
                &overlapped,
                INFINITE)) {

			if (totalBytes == 0) { // If number of bytes transferred is 0 connection has been closed.
				std::cout << "Connection Closing: " << pIoContext->m_connection << std::endl;
				pIoContext->Release(); // Releases reference to pIoContext a second time allowing connection to be dleleted
				break;
			}

			pIoContext->AddRef(); // Adds reference to IoContext object
			WSABUF *wsabuf = pIoContext->m_buffer->GetWSABUF(); // Stores pointer to buffer on stack for easier access.

			switch(pIoContext->m_ioEvent) {
				case initialRead: {
					pIoContext->m_nTotal = totalBytes; // Total bytes in buffer = total bytes received

					std::cout << pIoContext->m_nTotal << " bytes received: " << pIoContext->m_connection << std::endl;

					std::string message(wsabuf->buf);
					std::string acceptKey;
					std::vector<std::string> lines;

					int start = 0;
					int end	  = (int)message.find("\r\n");

					while (end != -1) {
						lines.push_back(message.substr(start, end - start)); // Separates lines of handshake request
						start = end + 2;
						end	  = (int)message.find("\r\n", start);
					}

					bool keyFound = false;

					for (std::string &line : lines) {
						if (line.find("Sec-WebSocket-Key: ") == 0) { // Finds the line with the WebSocketKey in it
							std::string key = line.substr(strlen("Sec-WebSocket-Key: ")); // Separates the key from the line
							keyFound = true;
							acceptKey = KeyCalc(key);
						}
					}

					if (!keyFound) {
						std::cerr << "WebSocket-Key could not be found" << std::endl;
						pIoContext->Release();
						break;
					}

					std::string response( // Setups handshake response
					  "HTTP/1.1 101 Switching Protocols\r\n"
					  "Upgrade: websocket\r\n"
					  "Connection: Upgrade\r\n"
					  "Sec-WebSocket-Accept: ");

					response.append(acceptKey + "\r\n\r\n"); // Appends key to response

					pIoContext->m_ioEvent = write; // Setups write event
					pIoContext->m_buffer->ClearBuf();
					pIoContext->m_buffer->AddData(response.c_str(),strlen(response.c_str()));


					int result = WSASend( // Sends handshake response
					  pIoContext->m_connection,
					  wsabuf,
					  1,
					  &(pIoContext->m_nTotal),
					  pIoContext->m_flags,
					  &(pIoContext->m_Overlapped),
					  nullptr);

					if (result != 0 && WSAGetLastError() != 997) {
						std::cerr << "WSASend() failed: " << WSAGetLastError() << std::endl;
					}

					pIoContext->Release();
					break;
				}
				case read: {
					pIoContext->m_nTotal = totalBytes;
					std::cout << pIoContext->m_nTotal << " bytes received: " << pIoContext->m_connection << std::endl;

					FrameFormat sendFormat = {0};
					sendFormat.fin = 1;

					std::string test(wsabuf->buf);

					uint64_t payloadLen = ReadBits(
					  wsabuf->buf[1], 1, 7); // Gets the value for the initial payload length

					uint8_t paylenSize = 0; // Initialised with 0, as if value of payloadLen is not 126/127 then no extra bytes need to be skipped in future processing

					if (payloadLen == 126) { // This check is explained in docs
						payloadLen = 0;
						paylenSize = 2;

						for (int i = 0; i < 2; i++) {
							payloadLen += (unsigned char)wsabuf->buf[2 + i];
							payloadLen = (payloadLen << (8 * (1 - i)));
						}
					}
					else if (payloadLen == 127) { // This check is explained in docs
						payloadLen = 0;
						paylenSize = 8;

						for (int i = 0; i < 8; i++) {
							payloadLen += (unsigned char)wsabuf->buf[2 + i];
							payloadLen = (payloadLen << (8 * (7 - i)));
						}
					}

					char mask[4];
					char payload[INIT_BUF_MEM];
					payload[payloadLen] = 0;

					if (ReadBits(wsabuf->buf[1], 0, 1)) { // Checks for mask bit
						sendFormat.mask = 1;
						memcpy(&mask, &wsabuf->buf[2 + paylenSize], 4); // Copies masking key to mask[4]

						for (int i = 0; i < payloadLen; i++) {
							payload[i] = wsabuf->buf[6 + paylenSize + i] ^ mask[i % 4]; // Unmasks payload
						}
					}
					else {
						memcpy(&payload, &wsabuf->buf[2 + paylenSize], payloadLen);
					}

					sendFormat.opcode = ReadBits(wsabuf->buf[0], 4, 4);
					pIoContext->m_buffer->ClearBuf();

					switch (sendFormat.opcode) {
						case 0: // Continuation frame, this will never be used so can be ignored
							std::cout << "Cont. frame received: " << pIoContext->m_connection << std::endl;

							break;
						case 1: { // Text frame, interpret data and send response back.
							std::cout << "Text frame received: " << pIoContext->m_connection << std::endl;

							std::string response = GenerateResponse(payload, pIoContext->m_db); // Generates response

							std::cout << response << std::endl;

							if (response.length() > 65535) { // If length > 2 bytes then payload length in the 2nd byte of this frame will = 127, check docs for explanation
								paylenSize = 6;
								sendFormat.payloadLen = 127;
								int64_t payloadLen = _byteswap_uint64(response.length());
								memcpy(&wsabuf->buf[2], &payloadLen, 8);
							} else if (response.length() > 125) { // If length > 7 bits but not > 2 bytes then payload length = 126
								paylenSize = 2;
								sendFormat.payloadLen = 126;
								uint16_t temp = response.length();
								temp = htons(temp);
								memcpy(&wsabuf->buf[2], &temp, 2);
							}
							else {
								paylenSize = 0;
								sendFormat.payloadLen = response.length();
							}

							memcpy(&wsabuf->buf[0], (uint16_t *)&sendFormat, 2); // Copies send format to first 2 bytes

							if (sendFormat.mask) { // Checks if masking bit should be on
								for (int i = 0; i < 4; i++) {
									std::random_device rd;
									std::mt19937 mt(rd());
									std::uniform_real_distribution<float> dist(0, 256);
									wsabuf->buf[2 + paylenSize + i] = dist(mt); // Generates random bytes using uniform distribution
								}

								for (int i = 0; i < response.length(); i++) { // Masks payload
									wsabuf->buf[6 + paylenSize + i] = response[i] ^ wsabuf->buf[2 + paylenSize + (i % 4)];
								}

								wsabuf->len = 6 + paylenSize + response.length();
							} else { // If no masking bit just copy payload
								memcpy(&wsabuf->buf[2 + paylenSize], &response[0], response.length());

								wsabuf->len = 2 + paylenSize + response.length();
							}

							pIoContext->m_nTotal = wsabuf->len;
							pIoContext->m_ioEvent = write;

							int result = WSASend( // Sends frame
							  pIoContext->m_connection,
							  wsabuf,
							  1,
							  nullptr,
							  pIoContext->m_flags,
							  &(pIoContext->m_Overlapped),
							  nullptr);

							if (result != 0 && WSAGetLastError() != 997) {
								std::cerr << "WSASend() failed: " << WSAGetLastError() << std::endl;
							}

							break;
						}
						case 2: // Binary frame, Will not be used
							std::cout << "Binary frame received: " << pIoContext->m_connection << std::endl;

							break;
						case 9: // Ping received. Return Pong with same payload and format.
							sendFormat.opcode = 10;
						case 8: { // Close frame received. Return Close frame and close connection.
							if (sendFormat.opcode == 8) {
								std::cout << "Close frame received: " << pIoContext->m_connection << std::endl;
							}
							else {
								std::cout << "Ping received: " << pIoContext->m_connection << std::endl;
							}

							memcpy(&wsabuf->buf[0], (uint16_t *)&sendFormat, 2);

							if (sendFormat.payloadLen == 127) {
								int64_t payloadLen = _byteswap_uint64(payloadLen);
								memcpy(&wsabuf->buf[2], &payloadLen, 8);
							} else if (sendFormat.payloadLen == 126) {
								uint16_t temp = payloadLen;
								temp		  = htons(temp);
								memcpy(&wsabuf->buf[2], &temp, 2);
							}

							if (sendFormat.mask) {
								for (int i = 0; i < 4; i++) {
									std::random_device rd;
									std::mt19937 mt(rd());
									std::uniform_real_distribution<float> dist(0.0, 256.0);
									wsabuf->buf[2 + paylenSize + i] = dist(mt);
								}

								for (int i = 0; i < payloadLen; i++) {
									wsabuf->buf[6 + paylenSize + i] =
									  payload[i] ^ wsabuf->buf[2 + paylenSize + (i % 4)];
								}

								wsabuf->len = 6 + paylenSize + payloadLen;
							} else {
								memcpy(&wsabuf->buf[2 + paylenSize], &payload, payloadLen);

								wsabuf->len = 2 + paylenSize + payloadLen;
							}


							pIoContext->m_ioEvent = write;

							int result = WSASend(pIoContext->m_connection,
												 wsabuf,
												 1,
												 nullptr,
												 pIoContext->m_flags,
												 &(pIoContext->m_Overlapped),
												 nullptr);

							if (result != 0 && WSAGetLastError() != 997) {
								std::cerr << "WSASend() failed: " << WSAGetLastError() << std::endl;
							}

							if (sendFormat.opcode == 8) {
								pIoContext->final = true; // sets final send on connection to true;
							}

							break;
						}
						case 10: // Pong received. Can be ignored.
							std::cout << "Pong Received: " << pIoContext->m_connection << std::endl;

						default:
							break;
					}

					pIoContext->Release();
					break;
				}
				case write: {
					std::cout << totalBytes << " bytes sent: " << pIoContext->m_connection << std::endl;

					pIoContext->m_nSent += totalBytes;

					if (pIoContext->m_nSent < pIoContext->m_nTotal) { // Checks if total bytes is less than sent bytes
						wsabuf->buf += pIoContext->m_nSent;
						wsabuf->len -= pIoContext->m_nSent;

						int result = WSASend( // If so send the rest of the data
						  pIoContext->m_connection,
						  wsabuf,
						  1,
						  nullptr,
						  pIoContext->m_flags,
						  &(pIoContext->m_Overlapped),
						  nullptr);

						if (result != 0 && WSAGetLastError() != 997) {
							std::cerr << "WSASend() failed: " << WSAGetLastError() << std::endl;
						}
					} else { // If all data sent, start receiving again
						pIoContext->m_ioEvent = read;
						pIoContext->m_buffer->ClearBuf();

						int result = WSARecv( // Calls receive on socket
						  pIoContext->m_connection,
						  wsabuf,
						  1,
						  &(pIoContext->m_nTotal),
						  &(pIoContext->m_flags),
						  &(pIoContext->m_Overlapped),
						  nullptr);

						if (result != 0 && WSAGetLastError() != 997) {
							std::cerr << "WSARecv() failed: " << WSAGetLastError() << std::endl;
						}
					}

					if (pIoContext->final) { // Checks if it is final send in connection
						pIoContext->Release(); // If so then release ref count allowing it to go to 0
					}

					pIoContext->Release();
					break;
				}
			}
        } else {
			std::cerr << "GetQueuedCompletionStatus() failed: " << GetLastError() << std::endl;
		}
    }
	return 0;
}

std::string Thread::KeyCalc(std::string key) { // Calculates WebSocketAccept Key
	key.append(WEB_SOCK_STRING);

	auto data = reinterpret_cast<const unsigned char*>(key.c_str());

	unsigned char keyHash[SHA_DIGEST_LENGTH];

	SHA1(data, key.size(), keyHash); // Hashes the key

	std::string binHash;

	for (unsigned char i : keyHash) { binHash.append(std::bitset<8>(i).to_string());} // Converts it to a bitset

	binHash.append("00"); // Append 2 0s to make it divis by 4

	char b64hash[28 + 1];

	for (int i = 0; i < 28; i++) {
		if (i*6 < binHash.size()) { // encodes hash in Base64
			int start = i*6;
			u_long bit6key = std::bitset<6>(binHash.substr(start, 6)).to_ulong();

			b64hash[i] = b64[bit6key];
		}
		else { // Adds padding if necessary
			b64hash[i] = '=';
		}
	}

	b64hash[28] = '\0'; // Sets last char to null terminator

	return b64hash;
}

uint8_t Thread::ReadBits(unsigned char c, uint8_t msb, uint8_t n) { // Read certain bits in bytes
	uint8_t total = 0;

	msb = 7 - msb;

	for (int i = 0; i < n; i++) {
		bool isOne = c & (1 << (msb -i));

		total += isOne << (n - i - 1);
	}
	return total;
}

bool compareWPM(const json& a, const json& b) { // Compares the WPM of a row in the leaderboard
	return a[1]["wpm"] > b[1]["wpm"];
}

std::string Thread::GenerateResponse(char payload[], Database *db) {
	json data;
	json response;

	try { // If payload passed is not in JSON format then just send back an error response
		data = json::parse(payload);
	} catch (json::parse_error& e) {
		std::cout << "Parse error: " << e.what() << std::endl;
		response["result"] = 0;
		response["errmsgs"] = {"Not in JSON Format"};
		return to_string(response);
	}

	int operation = data["operation"].get<int>(); // Setups response

	response["operation"] = operation; // Sets response operation = to received operation
	response["result"]	  = 1;
	response["errmsgs"]	  = {};

	switch (operation) {
		case 1: { // register operation
			std::string username = data["username"].get<std::string>();
			std::string email	 = data["email"].get<std::string>();
			std::string password = data["password"].get<std::string>();

			if (db->SelectCount("users", "username", username.c_str())) { // Checks if username exists
				response["result"] = 0;
				response["errmsgs"].push_back("Username taken.");
			}

			if (db->SelectCount("users", "email", email.c_str())) { // Checks if email exists
				response["result"] = 0;
				response["errmsgs"].push_back("Email already exists.");
			}

			if (!response["result"].get<int>()) { // If any of them already exist then return errors to user
				return to_string(response);
			} else {
				char saltBytes[8];

				for (char &saltByte : saltBytes) { // Generate salt of 8 random bytes
					std::random_device rd;
					std::mt19937 mt(rd());
					std::uniform_real_distribution<float> dist(0, 256);
					saltByte = dist(mt);
				}

				std::string saltHex =
				  ByteToHex<char *>(saltBytes, 8); // Convert byte array to hex representation

				std::cout << saltHex << std::endl;

				unsigned char pwordHash[SHA256_DIGEST_LENGTH];

				auto data = reinterpret_cast<const unsigned char *>((password + saltHex).c_str());

				std::string pwordHashHex = GenPWordHash(data); // Generates SHA-256 hash and converts it to hexadecimal string

				char *query =
				  "INSERT INTO users (username, email, pword_hash, salt) VALUES (?, ?, ?, ?);"; // Inserts values into user column
				std::vector<std::string> values = {username, email, pwordHashHex, saltHex};

				if (db->Insert(query, values)) {
					std::string sessionId = GenSessionID(db);

					query = "INSERT INTO sessions (session_id, user_id) SELECT ?, id FROM users WHERE username = ?;"; // Inserts values into sessions column
					values = {sessionId, username};

					if (!db->Insert(query, values)) {
						response["result"] = 0;
						response["errmsgs"].push_back("Server error: 2");
						return to_string(response);
					}

					json stats = { // Creates new stats config
					  {"username", username},
					  {"tests", 0},
					  {"avgwpm", 0.0},
					  {"bestwpm", 0.0},
					  {"avgacc", 0.0}
					};

					json testConfig = { // Creates new test config with default values
					  {"mode", "test"},
					  {"type", "time"},
					  {"number", 15}
					};

					json practiceConfig = json::array();

					for (int i = 0; i < 26; i++) { // Creates new practice config
						practiceConfig[i] = {0, 0, 0};
						/*
						 * 0: Stores average time taken to type each key
						 * 1: Stores the number of key presses
						 * 2: Stores the number of errors
						 */
					}

					// Inserts configs into user_config table
					query = "INSERT INTO user_config (user_id, stats, test_config, practice_config) SELECT id, ? as stats, ? as test_config, ? as practice_config FROM users WHERE username = ?;";
					values = {
					  to_string(stats),
					  to_string(testConfig),
					  to_string(practiceConfig),
					  username
					};

					if (db->Insert(query, values)) {
						response["sessionId"] = sessionId;
					}
					else {
						response["result"] = 0;
						response["errmsgs"].push_back("Server error: 2");
					}
				} else {
					response["result"] = 0;
					response["errmsgs"].push_back("Server error: 1");
				}
				break;
			}
		}
		case 2: { // Login operation
			std::string username = data["username"].get<std::string>();
			std::string password = data["password"].get<std::string>();

			if (db->SelectCount("users", "username", username.c_str())) { // Checks if user exists
				char *query		 = "SELECT salt FROM users WHERE username = ?;";
				std::string salt = db->SelectString(query, std::vector<std::string>({username})); // Gets salt from users table

				auto data = reinterpret_cast<const unsigned char *>((password + salt).c_str());

				std::string pwordHashHex = GenPWordHash(data); // Generates password hash with user inputted password

				query = "SELECT pword_hash FROM users WHERE username = ?;"; // Gets password hash stored in users table

				std::string dbPwordHash = db->SelectString(query, std::vector<std::string>({username}));

				if (pwordHashHex == dbPwordHash) { // Checks if they are the same
					std::string sessionId = GenSessionID(db);

					query = // Updates sessionId
					  "UPDATE sessions SET session_id = ? WHERE user_id = (SELECT user_id FROM users WHERE username = ?);";

					if (db->Update(query, std::vector<std::string>({sessionId, username}))) {
						response["sessionId"] = sessionId;
					}
					else {
						response["result"] = 0;
						response["errmsgs"].push_back("Username or password does not exist");
						return to_string(response);
					}
				} else {
					response["result"] = 0;
					response["errmsgs"].push_back("Username or password does not exist");
					return to_string(response);
				}
			} else {
				response["result"] = 0;
				response["errmsgs"].push_back("Username or password does not exist");
				return to_string(response);
			}
			break;
		}
		case 3: { // General Config request
			std::string query = "SELECT " + data["config"].get<std::string>() + " FROM user_config WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
			std::vector<std::string> values = {
			  data["sessionId"].get<std::string>()
			};

			std::string config = db->SelectString(query.c_str(), values);

			response["config"] = json::parse(config); // Return requested config
			break;
		}
		case 4: { // General Config update
			std::string query = "UPDATE user_config SET " + data["config"].get<std::string>() + " = ? WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
			std::vector<std::string> values = {
			  to_string(data["values"]),
			  data["sessionId"].get<std::string>()
			};

			if (!db->Update(query.c_str(), values)) {
				response["result"] = 0;
				response["errmsgs"].push_back("Server error, could not update config");
			}
			break;
		}
		case 5: { // Practice Word Set Update
			std::string sessionId = data["sessionId"].get<std::string>();

			// Gets practice config from user_config table
			char *query = "SELECT practice_config FROM user_config WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
			std::vector<std::string> values = {
			  sessionId
			};

			json add_config = data["addConfig"];
			json practice_config = json::parse(db->SelectString(query, values));

			for (int i = 0; i < 26; i++) { // Updates values in practice config
				json &practice_entry = practice_config[i];
				json &add_entry = add_config[i];

				unsigned int practicePresses = practice_entry[1].get<unsigned int>();
				unsigned int addPresses = add_entry[1].get<unsigned int>();

				unsigned int totalTime = (practice_entry[0].get<unsigned int>() * practicePresses) + (add_entry[0].get<unsigned int>() * addPresses);
				practice_entry[1] = practicePresses + addPresses;

				if (practice_entry[1].get<unsigned int>() != 0) {
					practice_entry[0] = totalTime / practice_entry[1].get<unsigned int>();
				}

				practice_entry[2] = practice_entry[2].get<unsigned int>() + add_entry[2].get<unsigned int>();
			}

			// Updates table's practice config with updated config
			query = "UPDATE user_config SET practice_config = ? WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
			values = {
			  to_string(practice_config),
			  sessionId
			};

			if (!db->Update(query, values)) {
				response["result"] = 0;
				response["errmsgs"].push_back("Server error, could not update practice config");
			}
			break;
		}
		case 6: { // Practice Word Set Request
			char *query =
			  "SELECT practice_config FROM user_config WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
			std::vector<std::string> values = {data["sessionId"].get<std::string>()};

			json practice_config = json::parse(db->SelectString(query, values));

			std::vector<std::string> words = GenPracticeWords(practice_config, data["number"].get<int>());

			response["words"] = words;
			break;
		}
		case 7: { // Test Key Scores Request
			json keyData = data["keyData"];

			std::array<double, 26> keyScores = GenScores(keyData);

			response["keyScores"] = keyScores;
			break;
		}
		case 8: { // Overall Key Scores and Practice Config Request
			char *query =
			  "SELECT practice_config FROM user_config WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
			std::vector<std::string> values = {data["sessionId"].get<std::string>()};

			json keyData = json::parse(db->SelectString(query, values));

			std::array<double, 26> keyScores = GenScores(keyData);

			response["keyScores"] = keyScores;
			response["keyData"] = keyData;

			break;
		}
		case 9: { // Test Upload
			std::string sessionId = data["sessionId"].get<std::string>();
			std::string type = data["type"].get<std::string>();
			std::string number = data["number"].get<std::string>();

			// Uploads completed test to test table
			char *query = "INSERT INTO tests (user_id, type, number, test_data) SELECT user_id, ? as type, ? as number, ? as test_data FROM sessions WHERE session_id = ?;";
			std::vector<std::string> values = {
			  type,
			  number,
			  to_string(data["test_data"]),
			  sessionId
			};

			if (db->Insert(query, values)) {

				query = "SELECT stats FROM user_config WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
				values = {
				  sessionId
				};

				json stats = json::parse(db->SelectString(query, values));

				// Updates stats config with test values
				stats["avgwpm"] = (stats["avgwpm"].get<double>() * stats["tests"].get<double>() + data["test_data"]["wpm"].get<double>()) / (stats["tests"].get<double>() + 1);
				stats["avgacc"] = (stats["avgacc"].get<double>() * stats["tests"].get<double>() + data["test_data"]["accuracy"].get<double>()) / (stats["tests"].get<double>() + 1);

				stats["tests"] = stats["tests"].get<int>() + 1;

				// Checks if completed wpm is better than best wpm
				if (data["test_data"]["wpm"].get<double>() > stats["bestwpm"].get<double>()) {
					stats["bestwpm"] = data["test_data"]["wpm"].get<double>();
				}

				// Updates stats config in user_config table with updated stats config
				query = "UPDATE user_config SET stats = ? WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
				values = {
				  to_string(stats),
				  sessionId
				};

				db->Update(query, values);

				// Selects test_data from leaderboard
				query = "SELECT test_data FROM leaderboard WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?) AND type = ? AND number = ?;";
				values = {
				  sessionId,
				  type,
				  number
				};

				std::string data_string = db->SelectString(query, values);

				// Checks if there was a leaderboard entry
				if (data_string == "null") {
					// If not then insert completed test data
					query = "INSERT INTO leaderboard (user_id, username, test_data, type, number) SELECT sessions.user_id, users.username, ? AS test_data, ? AS type, ? AS number FROM sessions JOIN users ON sessions.user_id = users.id WHERE sessions.session_id = ?;";
					values = {
					  to_string(data["test_data"]),
					  type,
					  number,
					  sessionId
					};

					if (!db->Insert(query, values)) {
						response["result"] = 0;
						response["errmsgs"].push_back("Could not upload completed test to leaderboard");
					}
				}
				else {
					json leaderboard_data = json::parse(data_string);

					// Checks if test wpm > leaderboard wpm
					if (data["test_data"]["wpm"].get<double>() > leaderboard_data["wpm"].get<double>()) {
						// If it is then update the leaderboard entry with better
						query = "UPDATE leaderboard SET date_completed = datetime('now'), test_data = ? WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?) AND type = ? AND number = ?;";
						values = {
						  to_string(data["test_data"]),
						  sessionId,
						  type,
						  number
						};

						if (!db->Update(query, values)) {
							response["result"] = 0;
							response["errmsgs"].push_back("Could not upload completed test to leaderboard");
						}
					}
				}
			}
			else {
				response["result"] = 0;
				response["errmsgs"].push_back("Server error, could not upload test data");
			}

			break;
		}
		case 10: { // Leaderboard Request
			// Selects username and test_data from leaderboard table where type and number = user input
			char *query = "SELECT username, test_data FROM leaderboard WHERE type = ? and number = ?;";
			std::vector<std::string> values = {
			  data["type"].get<std::string>(),
			  data["number"].get<std::string>()
			};

			std::vector<std::string> rows = db->SelectMultiple(query, values, 2);

			// Checks if there are any leaderboard entries
			if (!rows.empty()) {
				json jsonRows = json::array();

				// If there are then parse the data and add it to the json array
				for (size_t i = 0; i < rows.size(); i += 2) {
					std::string username = rows[i];
					json test_data = json::parse(rows[i + 1]);
					jsonRows.push_back({username, test_data});
				}

				// Orders the data by wpm
				std::sort(jsonRows.begin(), jsonRows.end(), compareWPM);

				response["rows"] = jsonRows;
			}
			else {
				response["result"] = 0;
				response["errmsgs"].push_back("No entries in leaderboard");
			}
			break;
		}
	}
	return to_string(response);
}

std::string Thread::GenPWordHash(const unsigned char *data) {
	unsigned char pwordHash[SHA256_DIGEST_LENGTH];

	SHA256(data, strlen((char*)data), pwordHash); // hashes the data with SHA256

	std::string pwordHashHex = ByteToHex<unsigned char *>(pwordHash, 32); // Converts it to a hexadecimal string

	return pwordHashHex;
}

std::string Thread::GenSessionID(Database *db) {
	char sessionIdBytes[16];

	for (char &byte : sessionIdBytes) { // Generate salt of 8 random bytes
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_real_distribution<float> dist(0, 256);
		byte = dist(mt);
	}

	std::string sessionId = ByteToHex(sessionIdBytes, 16);
	/*
	 * Checks if there is already a sessionId with the same value assigned.
	 * Extremely improbable as there is ~3.4e+38 possible values for the sessionId,
	 * but will cause problems if duplicates occur.
	 */
	while (db->SelectCount("sessions", "session_id", sessionId.c_str())) {
		for (char &byte : sessionIdBytes) { // Generate salt of 8 random bytes
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_real_distribution<float> dist(0, 256);
			byte = dist(mt);
		}
		sessionId = ByteToHex(sessionIdBytes, 16);
	}
	return sessionId;
}

struct ComparePair {
	bool operator()(const std::pair<double, std::string> &a, const std::pair<double, std::string> &b) {
		return a.first > b.first;
	}
};

std::array<double, 26> Thread::GenScores(const json &keyData) {
	std::array<double, 26> scores = {0}; // array to store the scores for each key

	double totalTime = 0, totalErrors = 0;

	double totalKeys = 26;

	for (const json &entry : keyData) {
		unsigned int time = entry[0].get<unsigned int>();

		if (entry[1].get<unsigned int>() == 0) {
			totalKeys--; // If keys were never pressed then this stops them from bringing down the average
		}

		totalTime += time;
	}

	double avgTime = totalTime / totalKeys;

	double sosPosTimeD	   = 0; // sum of squares of negative time differences
	unsigned int sosErrors = 0; // sum of squares of errors

	for (const json &entry : keyData) {
		unsigned int time = entry[0].get<unsigned int>();
		unsigned int errors = entry[2].get<unsigned int>();

		double timeD = time - avgTime; // Gets timeD between key's avg time and global time

		if (timeD > 0) { // If time is positive then square it and add to sum of squares of timeDs
			sosPosTimeD += timeD * timeD;
		}
		sosErrors += errors * errors;
	}

	for (int i = 0; i < 26; i++) {
		const json &entry = keyData[i];

		unsigned int time = entry[0].get<unsigned int>();
		unsigned int errors = entry[2].get<unsigned int>();

		double timeD = time - avgTime;

		double sqrTimeD = 0;

		if (timeD > 0) {
			sqrTimeD = timeD * timeD;
		}
		double sqrErrors = errors * errors;

		scores[i] = ((sqrTimeD / sosPosTimeD) * 0.25) + ((sqrErrors / sosErrors) * 0.75); // Weights the scores 25/75 in favour of errors
	}

	return scores;
}

std::vector<std::string> Thread::GenPracticeWords(const json &practice_config, int number) {
	std::array<double, 26> scores = GenScores(practice_config); // calls gen scores on practice set

	std::vector<std::pair<double, char>> posScores; // Vector of integer representation of chars with positive scores

	for (int i = 0; i < 26; i++) {
		if (scores[i] > 0) {
			posScores.emplace_back(scores[i], static_cast<char>(97+i));
		}
	}

	json words;

	std::ifstream f("C:\\Users\\joshi\\CLionProjects\\WebSockIOCP\\words_5k.json"); // 5000 word json file, did not include for brevity
	words = json::parse(f);
	f.close();

	json wordsArray = words["words"];

	std::vector<std::pair<double, std::string>> wordScores;

	for (const auto &word : wordsArray) {
		wordScores.emplace_back(0.0, word.get<std::string>());
		for (const char &c : word.get<std::string>()) { // For every character in the word
			for (const std::pair<double, char> &pair : posScores) {
				if ((c & pair.second) == c) { // bitwise & posScore character with word's character and checks for character match
					wordScores[wordScores.size() - 1].first += pair.first; // Adds the score of the posScore character to the word's score
					break; // No other characters can return a value > 0 for this character so just skip them
				}
			}
		}
		wordScores[wordScores.size() - 1].first /= double(word.get<std::string>().length()); // Divide the score by the word length to remove long word bias
	}


	std::priority_queue<std::pair<double, std::string>, std::vector<std::pair<double, std::string>>, ComparePair> minHeap;

	//Sorts the scores in descending order and only keeps the number requested
	// Using a minHeap queue which makes sure the smallest score is always at the front of the queue
	for (const std::pair<double, std::string> &pair : wordScores) {
		if (minHeap.size() < number && pair.first != 0) {
			minHeap.push(pair);
		}
		else if (pair.first > minHeap.top().first) {
			minHeap.pop();
			minHeap.push(pair);
		}
	}

	std::vector<std::string> practiceWords;

	// Empties
	while (!minHeap.empty()) {
		practiceWords.push_back(minHeap.top().second);
		minHeap.pop();
	}

	return practiceWords;
}



bool Thread::Terminate() {
    if (TerminateThread(
            m_handle,
            0) == 0) {
        std::cerr << "TerminateThread() failed on thread " << m_threadId << " with: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}