#include <thread.hpp>

using json = nlohmann::json;

char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char hexmap[] = "0123456789abcdef";

Thread::Thread(HANDLE iocPort) : m_handle(INVALID_HANDLE_VALUE), m_threadId(0) {
    m_handle = CreateThread( // Creates thread
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
						lines.push_back(message.substr(start, end - start));
						start = end + 2;
						end	  = (int)message.find("\r\n", start);
					}

					bool keyFound = false;

					for (std::string &line : lines) {
						if (line.find("Sec-WebSocket-Key: ") == 0) {
							std::string key = line.substr(strlen("Sec-WebSocket-Key: "));
							keyFound = true;
							acceptKey = KeyCalc(key);
						}
					}

					if (!keyFound) {
						std::cerr << "WebSocket-Key could not be found" << std::endl;
						pIoContext->Release();
						break;
					}

					std::string response(
					  "HTTP/1.1 101 Switching Protocols\r\n"
					  "Upgrade: websocket\r\n"
					  "Connection: Upgrade\r\n"
					  "Sec-WebSocket-Accept: ");

					response.append(acceptKey + "\r\n\r\n");

					pIoContext->m_ioEvent = write;
					pIoContext->m_buffer->ClearBuf();
					pIoContext->m_buffer->AddData(response.c_str(),strlen(response.c_str()));


					int result = WSASend(
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

					char mask[4 + 1];
					char payload[INIT_BUF_MEM];
					payload[payloadLen] = 0;

					if (ReadBits(wsabuf->buf[1], 0, 1)) {
						sendFormat.mask = 1;
						mask[4] = 0;
						memcpy(&mask, &wsabuf->buf[2 + paylenSize], 4);

						for (int i = 0; i < payloadLen; i++) {
							payload[i] = wsabuf->buf[6 + paylenSize + i] ^ mask[i % 4];
						}
					}
					else {
						memcpy(&payload, &wsabuf->buf[2 + paylenSize], payloadLen);
					}

					sendFormat.opcode = ReadBits(wsabuf->buf[0], 4, 4);
					pIoContext->m_buffer->ClearBuf();

					switch (sendFormat.opcode) {
						case 0: // Continuation frame, add to payload data.
							std::cout << "Cont. frame received: " << pIoContext->m_connection << std::endl;

							break;
						case 1: { // Text frame, interpret data and send response back.
							std::cout << "Text frame received: " << pIoContext->m_connection << std::endl;

							std::string response = GenerateResponse(payload, pIoContext->m_db);

							if (response.length() > 65535) {
								paylenSize = 6;
								sendFormat.payloadLen = 127;
								int64_t payloadLen = _byteswap_uint64(response.length());
								memcpy(&wsabuf->buf[2], &payloadLen, 8);
							} else if (response.length() > 125) {
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

							memcpy(&wsabuf->buf[0], (uint16_t *)&sendFormat, 2);

							if (sendFormat.mask) {
								for (int i = 0; i < 4; i++) {
									std::random_device rd;
									std::mt19937 mt(rd());
									std::uniform_real_distribution<float> dist(0, 256);
									wsabuf->buf[2 + paylenSize + i] = dist(mt);
								}

								for (int i = 0; i < response.length(); i++) {
									wsabuf->buf[6 + paylenSize + i] = response[i] ^ wsabuf->buf[2 + paylenSize + (i % 4)];
								}

								wsabuf->len = 6 + paylenSize + response.length();
							} else {
								memcpy(&wsabuf->buf[2 + paylenSize], &response[0], response.length());

								wsabuf->len = 2 + paylenSize + response.length();
							}

							pIoContext->m_nTotal = wsabuf->len;
							pIoContext->m_ioEvent = write;

							int result = WSASend(
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
						case 2: // Binary frame, Will not be used, data is echoed back.
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
								pIoContext->final = 1;
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

					if (pIoContext->m_nSent < pIoContext->m_nTotal) {
						wsabuf->buf += pIoContext->m_nSent;
						wsabuf->len -= pIoContext->m_nSent;

						int result = WSASend(
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
					} else {
						pIoContext->m_ioEvent = read;
						pIoContext->m_buffer->ClearBuf();

						int result = WSARecv(pIoContext->m_connection,
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

					if (pIoContext->final) {
						pIoContext->Release();
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

std::string Thread::KeyCalc(std::string key) {
	key.append(WEB_SOCK_STRING);

	auto data = reinterpret_cast<const unsigned char*>(key.c_str());

	unsigned char keyHash[SHA_DIGEST_LENGTH];

	SHA1(data, key.size(), keyHash);

	std::string binHash;

	for (unsigned char i : keyHash) { binHash.append(std::bitset<8>(i).to_string());}

	binHash.append("00");

	char b64hash[28 + 1];

	for (int i = 0; i < 28; i++) {
		if (i*6 < binHash.size()) {
			int start = i*6;
			u_long bit6key = std::bitset<6>(binHash.substr(start, 6)).to_ulong();

			b64hash[i] = b64[bit6key];
		}
		else {
			b64hash[i] = '=';
		}
	}

	b64hash[28] = '\0';

	return b64hash;
}

uint8_t Thread::ReadBits(unsigned char c, uint8_t msb, uint8_t n) {
	uint8_t total = 0;

	msb = 7 - msb;

	for (int i = 0; i < n; i++) {
		bool isOne = c & (1 << (msb -i));

		total += isOne << (n - i - 1);
	}
	return total;
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

	int operation = data["operation"].get<int>();

	response["operation"] = operation;
	response["result"]	  = 1;
	response["errmsgs"]	  = {};

	switch (operation) {
		case 1: { // register operation
			std::string username = data["username"].get<std::string>();
			std::string email	 = data["email"].get<std::string>();
			std::string password = data["password"].get<std::string>();

			if (db->SelectCount("users", "username", username.c_str())) {
				response["result"] = 0;
				response["errmsgs"].push_back("Username taken.");
			}

			if (db->SelectCount("users", "email", email.c_str())) {
				response["result"] = 0;
				response["errmsgs"].push_back("Email already exists.");
			}

			if (!response["result"].get<int>()) {
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

				SHA256(data, password.size(), pwordHash);

				std::string pwordHashHex = ByteToHex<unsigned char *>(pwordHash, 32);

				char *query =
				  "INSERT INTO users (username, email, pword_hash, salt) VALUES (?, ?, ?, ?);";
				std::vector<std::string> values = {username, email, pwordHashHex, saltHex};

				if (db->Insert(query, values)) {
					std::string sessionId = GenSessionID(db);

					query = "INSERT INTO sessions (session_id, user_id) SELECT ?, id FROM users WHERE username = ?;";
					values = {sessionId, username};

					if (!db->Insert(query, values)) {
						response["result"] = 0;
						response["errmsgs"].push_back("Server error: 2");
						return to_string(response);
					}

					json stats = {
					  {"tests", 0},
					  {"words", 0},
					  {"letters", 0},
					  {"worstl", nullptr},
					  {"bestl", nullptr}
					};

					json testConfig = {
					  {"mode", "test"},
					  {"type", "time"},
					  {"number", 15}
					};

					json practiceConfig = json::array();

					for (int i = 0; i < 26; i++) {
						practiceConfig[i] = {0, 0, 0};
						/*
						 * 0: Stores average time taken to type each key
						 * 1: Stores the number of key presses
						 * 2: Stores the number of errors
						 */
					}

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

			if (db->SelectCount("users", "username", username.c_str())) {
				char *query		 = "SELECT salt FROM users WHERE username = ?;";
				std::string salt = db->SelectString(query, std::vector<std::string>({username}));

				auto data = reinterpret_cast<const unsigned char *>((password + salt).c_str());

				std::string pwordHashHex = GenPWordHash(data);

				query = "SELECT pword_hash FROM users WHERE username = ?;";

				std::string dbPwordHash = db->SelectString(query, std::vector<std::string>({username}));

				if (pwordHashHex == dbPwordHash) {
					std::string sessionId = GenSessionID(db);

					query =
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

			response["config"] = json::parse(config);
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

			char *query = "SELECT practice_config FROM user_config WHERE user_id = (SELECT user_id FROM sessions WHERE session_id = ?);";
			std::vector<std::string> values = {
			  sessionId
			};

			json add_config = data["addConfig"];
			json practice_config = json::parse(db->SelectString(query, values));

			for (int i = 0; i < 26; i++) {
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
	}
	return to_string(response);
}

std::string Thread::GenPWordHash(const unsigned char *data) {
	unsigned char pwordHash[SHA256_DIGEST_LENGTH];

	SHA256(data, strlen((char*)data), pwordHash);

	std::string pwordHashHex = ByteToHex<unsigned char *>(pwordHash, 32);

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

std::vector<std::string> Thread::GenPracticeWords(const json &practice_config, int number) {
	double scores[26] = {0}; // array to store the scores for each key

	double totalTime = 0, totalErrors = 0;

	double totalKeys = 26;

	for (const json &entry : practice_config) {
		unsigned int time = entry[0].get<unsigned int>();

		if (entry[1].get<unsigned int>() == 0) {
			totalKeys--; // If keys were never pressed then this stops them from bringing down the average
		}

		totalTime += time;
	}

	double avgTime = totalTime / totalKeys;

	double sosNegTimeD = 0; // sum of squares of negative time differences
	unsigned int sosErrors = 0; // sum of squares of errors

	for (const json &entry : practice_config) {
		unsigned int time = entry[0].get<unsigned int>();
		unsigned int errors = entry[2].get<unsigned int>();

		double timeD = time - avgTime;

		if (timeD > 0) {
			sosNegTimeD += timeD * timeD;
		}
		sosErrors += errors * errors;
	}

	for (int i = 0; i < 26; i++) {
		const json &entry = practice_config[i];

		unsigned int time = entry[0].get<unsigned int>();
		unsigned int errors = entry[2].get<unsigned int>();

		double timeD = time - avgTime;

		double sqrTimeD = 0;

		if (timeD > 0) {
			sqrTimeD = timeD * timeD;
		}
		double sqrErrors = errors * errors;

		scores[i] = ((sqrTimeD / sosNegTimeD) * 0.25) + (sqrErrors / sosErrors);
	}

	std::vector<std::pair<double, char>> posScores; // Vector of integer representation of chars with positive scores

	for (int i = 0; i < 26; i++) {
		if (scores[i] > 0) {
			posScores.emplace_back(scores[i], static_cast<char>(97+i));
		}
	}

	json words;

	std::ifstream f("C:\\Users\\joshi\\CLionProjects\\WebSockIOCP\\words_5k.json");
	words = json::parse(f);
	f.close();

	json wordsArray = words["words"];

	std::vector<std::pair<double, std::string>> wordScores;

	for (const auto &word : wordsArray) {
		wordScores.emplace_back(0.0, word.get<std::string>());
		for (const char &c : word.get<std::string>()) {
			for (const std::pair<double, char> &pair : posScores) {
				if ((c & pair.second) == c) {
					wordScores[wordScores.size() - 1].first += pair.first;
					break; // No other characters can return a value > 0 for this character so just skip them
				}
			}
		}
		wordScores[wordScores.size() - 1].first /= double(word.get<std::string>().length());
	}


	std::priority_queue<std::pair<double, std::string>, std::vector<std::pair<double, std::string>>, ComparePair> minHeap;

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