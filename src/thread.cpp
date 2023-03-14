#include <thread.hpp>
#include <openssl/sha.h>
#include <bitset>
#include <cmath>

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
    DWORD totalBytes;
    IoContext *ioContext;
    LPWSAOVERLAPPED overlapped;

    while (true) {
        if (GetQueuedCompletionStatus(
                iocpHandle,
                &totalBytes,
                (PULONG_PTR)&ioContext,
                &overlapped,
                INFINITE)) {
			std::string message(ioContext->m_wsabuf.buf);

			std::cout << ioContext->m_pConnection->GetHandle() << std::endl;

			switch(ioContext->m_ioEvent) {
				case initialRead:
					std::string acceptKey;

					std::vector<std::string> lines;

					int start = 0;
					int end = (int)message.find("\r\n");

					while (end != -1) {
						lines.push_back(message.substr(start, end - start));
						start = end + 2;
						end = (int)message.find("\r\n", start);
					}

					for (std::string& line : lines) {
						if (line.find("Sec-WebSocket-Key: ") == 0) {
							std::string key = line.substr(strlen("Sec-WebSocket-Key: "));

							acceptKey = KeyCalc(key);
						}
					}
					std::string response(
					  "HTTP/1.1 101 Switching Protocols\r\n"
					  "Upgrade: websocket\r\n"
					  "Connection: Upgrade\r\n"
					  "Sec-WebSocket-Accept: ");

					response.append(acceptKey);

					std::cout << acceptKey.c_str() << std::endl;

					std::cout << response << std::endl;
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