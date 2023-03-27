#include <thread.hpp>

using json = nlohmann::json;

Thread::Thread(HANDLE iocPort) : m_handle(INVALID_HANDLE_VALUE), m_threadId(0) {
    m_handle = CreateThread( // Creates thread
	  nullptr,
	  0,
	  Thread::IoWork,
	  iocPort,
	  0,
	  &m_threadId);

    if (m_handle == INVALID_HANDLE_VALUE) {
        std::cout << "CreateThread() failed: " << GetLastError() << std::endl;
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
						std::cout << "WebSocket-Key could not be found" << std::endl;
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
						std::cout << "WSASend() failed: " << WSAGetLastError() << std::endl;
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
					sendFormat.payloadLen = payloadLen; // Set the 7 bit payload length for the send frame.

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

							memcpy(&wsabuf->buf[0], (uint16_t *)&sendFormat, 2);

							GenerateResponse(payload, pIoContext->m_db);

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
									std::uniform_real_distribution<float> dist(0, 256);
									wsabuf->buf[2 + paylenSize + i] = dist(mt);
								}

								for (int i = 0; i < payloadLen; i++) {
									wsabuf->buf[6 + paylenSize + i] = payload[i] ^ wsabuf->buf[2 + paylenSize + (i % 4)];
								}

								wsabuf->len = 6 + paylenSize + payloadLen;
							} else {
								memcpy(&wsabuf->buf[2 + paylenSize], &payload, payloadLen);

								wsabuf->len = 2 + paylenSize + payloadLen;
							}

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
								std::cout << "WSASend() failed: " << WSAGetLastError() << std::endl;
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

							if (sendFormat.opcode == 8) {
								pIoContext->m_ioEvent = close;
							}
							else {
								pIoContext->m_ioEvent = write;
							}

							int result = WSASend(pIoContext->m_connection,
												 wsabuf,
												 1,
												 nullptr,
												 pIoContext->m_flags,
												 &(pIoContext->m_Overlapped),
												 nullptr);

							if (result != 0 && WSAGetLastError() != 997) {
								std::cout << "WSASend() failed: " << WSAGetLastError() << std::endl;
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
							std::cout << "WSASend() failed: " << WSAGetLastError() << std::endl;
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
							std::cout << "WSARecv() failed: " << WSAGetLastError() << std::endl;
						}
					}

					pIoContext->Release();
					break;
				}
				case close: {
					pIoContext->Release();

					break;
				}
			}
        } else {
			std::cout << GetLastError() << std::endl;
		}
    }
}

std::string Thread::KeyCalc(std::string key) {
	key.append(WEB_SOCK_STRING);

	auto data = reinterpret_cast<const unsigned char*>(key.c_str());

	unsigned char keyHash[SHA_DIGEST_LENGTH];

	SHA1(data, key.size(), keyHash);

	std::string binHash;

	for (unsigned char i : keyHash) { binHash.append(std::bitset<8>(i).to_string());}

	binHash.append("00");

	char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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

char *Thread::GenerateResponse(char payload[], Database *db) {
	json data = json::parse(payload);

	if (data["operation"] == "register") {
		char salt[16];

		for (char & i : salt) {
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_real_distribution<float> dist(0.0, 256.0);
			i = dist(mt);
		}

	}

	return "test";
}

bool Thread::Terminate() {
    if (TerminateThread(
            m_handle,
            0) == 0) {
        std::cout << "TerminateThread() failed on thread " << m_threadId << " with: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}
