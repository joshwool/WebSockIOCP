#ifndef WEBSOCKIOCP_THREAD_HPP
#define WEBSOCKIOCP_THREAD_HPP

#include <core.hpp>

#include <iocontext.hpp>

extern char hexmap[];

class Threadpool;

typedef struct FrameFormat { // Frame format explained further in docs
	unsigned char opcode : 4;
	unsigned char : 3; // Not using any websocket extensions, so all RSV bits will always be 0.
	unsigned char fin : 1;
	unsigned char payloadLen : 7;
	unsigned char mask : 1;
} FrameFormat, pFrameFormat;

class Thread {
public:
    explicit Thread(HANDLE iocPort);
    ~Thread();

    static DWORD WINAPI IoWork(LPVOID lpParam); // Work function ran by threads

	static std::string KeyCalc(std::string key); // Calculates the Web-Socket-Accept Key

	static uint8_t ReadBits(unsigned char c, uint8_t msb, uint8_t n); // Reads specific bits in a byte and returns the value.

	static std::string GenerateResponse(char payload[], Database *db); // Generates response from payload

	template<typename T> static std::string ByteToHex(T bytes, uint8_t length) {
		std::string hex(length * 2, ' ');

		for (int i = 0; i < length; i++) {
			hex[i * 2] = hexmap[(bytes[i] & 0xF0) >> 4];
			hex[i * 2 + 1] = hexmap[bytes[i] & 0x0F];
		}

		return hex;
	}

	static std::string GenPWordHash(const unsigned char* data);
	static std::string GenSessionID(Database *db);

    bool Terminate(); // Terminates current thread
private:
    HANDLE m_handle;
    DWORD m_threadId;
};

#endif //WEBSOCKIOCP_THREAD_HPP
