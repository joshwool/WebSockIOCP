#include <thread.hpp>

Thread::Thread(HANDLE iocPort) : m_handle(INVALID_HANDLE_VALUE), m_threadId(0) {
    m_handle = CreateThread(
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
		DWORD totalBytes;
		DWORD sentBytes;
		IoContext *pIoContext;
		LPWSAOVERLAPPED overlapped;

        if (GetQueuedCompletionStatus(
                iocpHandle,
                &totalBytes,
                (PULONG_PTR)&pIoContext,
                &overlapped,
                INFINITE)) {

			if (totalBytes == 0) {
				std::cout << "Connection Closing: " << pIoContext->m_connection << std::endl;
				pIoContext->Release();
				break;
			}

			pIoContext->AddRef();
			WSABUF *wsabuf = pIoContext->m_buffer->GetWSABUF();

			switch(pIoContext->m_ioEvent) {
				case initialRead: {
					pIoContext->m_nTotal = totalBytes;

					std::cout << pIoContext->m_nTotal << " bytes received" << std::endl;

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
						closesocket(pIoContext->m_connection);
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

					std::cout << pIoContext->m_nTotal << " bytes received" << std::endl;

					std::string message(wsabuf->buf);

					for (int i = 0; i < totalBytes; i++) { std::cout << std::bitset<8>(wsabuf->buf[i]).to_string(); }
					std::cout << std::endl;

					uint64_t payloadLen = readBits(wsabuf->buf[1], 1, 7);

					if (payloadLen == 126) {
						uint16_t payloadLen;

						memcpy(&payloadLen, &wsabuf->buf[2], 2);
						payloadLen = htons(payloadLen);
					}
					else if (payloadLen == 127) {
						uint64_t payloadLen;

						memcpy(&payloadLen, &wsabuf->buf[2], 8);
						payloadLen = _byteswap_uint64(payloadLen);
					}



					char payload[payloadLen];




					switch (readBits(wsabuf->buf[0], 4, 4)) {
						case 0:

							break;
						case 1:
							if (readBits(wsabuf->buf[0], 0, 1) == 0) {
								pIoContext->m_opcode = 1;
							}
							break;
						case 2:
							if (readBits(wsabuf->buf[0], 0, 1) == 0) {
								pIoContext->m_opcode = 1;
							}


							break;

						case 8:

							break;

						case 9:

							break;

						default:
							break;
					}

					pIoContext->Release();
					break;
				}
				case write: {
					std::cout << totalBytes << " bytes sent" << std::endl;

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

	for (unsigned char i : keyHash) { binHash.append(std::bitset<8>(i).to_string());
	}

	binHash.append("00");

	char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	char b64hash[28];

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

uint8_t Thread::readBits(const unsigned char c, uint8_t msb, uint8_t n) {
	uint8_t total = 0;

	msb = 7 - msb;

	for (int i = 0; i < n; i++) {
		bool isOne = c & (1 << (msb -i));

		total += isOne << (n - i - 1);
	}
	return total;
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
