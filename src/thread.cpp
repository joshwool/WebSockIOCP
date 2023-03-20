#include <thread.hpp>
#include <openssl/sha.h>
#include <bitset>

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
		IoContext *ioContext;
		LPWSAOVERLAPPED overlapped;

        if (GetQueuedCompletionStatus(
                iocpHandle,
                &totalBytes,
                (PULONG_PTR)&ioContext,
                &overlapped,
                INFINITE)) {

			if (totalBytes == 0) {
				std::cout << "Connection Closing: " << ioContext->m_connection << std::endl;
				ioContext->Release();
				break;
			}

			ioContext->AddRef();

			switch(ioContext->m_ioEvent) {
				case initialRead: {
					std::string message(ioContext->m_wsabuf.buf);

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
						closesocket(ioContext->m_connection);
						ioContext->Release();
						break;
					}

					std::string response(
					  "HTTP/1.1 101 Switching Protocols\r\n"
					  "Upgrade: websocket\r\n"
					  "Connection: Upgrade\r\n"
					  "Sec-WebSocket-Accept: ");

					response.append(acceptKey + "\r\n\r\n");

					ioContext->m_ioEvent = write;
					ioContext->m_wsabuf.buf = const_cast<char *>(response.c_str());
					ioContext->m_wsabuf.len = strlen(response.c_str());

					int result = WSASend(
					  ioContext->m_connection,
					  &(ioContext->m_wsabuf),
					  1,
					  &(ioContext->m_nTotal),
					  ioContext->m_flags,
					  &(ioContext->m_Overlapped),
					  nullptr);

					if (result != 0 && WSAGetLastError() != 997) {
						std::cout << "WSASend() failed: " << WSAGetLastError() << std::endl;
					} else {
						std::cout << ioContext->m_nTotal << " bytes sent" << std::endl;
					}

					ioContext->Release();
					break;
				}
				case read: {
					std::string message(ioContext->m_wsabuf.buf);

					std::cout << message << std::endl;

					break;
				}
				case write: {
					ioContext->m_nSent += totalBytes;

					if (ioContext->m_nSent < ioContext->m_nTotal) {

						ioContext->m_wsabuf.buf += ioContext->m_nSent;
						ioContext->m_wsabuf.len -= ioContext->m_nSent;

						int result = WSASend(
						  ioContext->m_connection,
						  &(ioContext->m_wsabuf),
						  1,
						  &sentBytes,
						  ioContext->m_flags,
						  &(ioContext->m_Overlapped),
						  nullptr);

						if (result != 0 && WSAGetLastError() != 997) {
							std::cout << "WSASend() failed: " << WSAGetLastError() << std::endl;
						} else {
							std::cout << sentBytes << " bytes sent" << std::endl;
						}
					} else {
						ioContext->m_ioEvent = read;
						ioContext->m_buffer->ClearBuf();
						ioContext->m_wsabuf = *ioContext->m_buffer->GetWSABUF();

						int result = WSARecv(
						  ioContext->m_connection,
						  &(ioContext->m_wsabuf),
						  1,
						  &(ioContext->m_nTotal),
						  &(ioContext->m_flags),
						  &(ioContext->m_Overlapped),
						  nullptr);

						if (result != 0 && WSAGetLastError() != 997) {
							std::cout << "WSARecv() failed: " << WSAGetLastError() << std::endl;
						} else {
							std::cout << ioContext->m_nTotal << " bytes received" << std::endl;
						}
					}

					ioContext->Release();
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

	std::string binKey;

	for (unsigned char i : keyHash) {
		binKey.append(std::bitset<8>(i).to_string());
	}

	binKey.append("00");

	char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	char finKey[28];

	for (int i = 0; i < 28; i++) {
		if (i*6 < binKey.size()) {
			int start = i*6;
			u_long bit6key = std::bitset<6>(binKey.substr(start, 6)).to_ulong();

			finKey[i] = b64[bit6key];
		}
		else {
			finKey[i] = '=';
		}
	}

	finKey[28] = '\0';

	return finKey;
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