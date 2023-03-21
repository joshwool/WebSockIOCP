#ifndef WEBSOCKIOCP_IOCONTEXT_HPP
#define WEBSOCKIOCP_IOCONTEXT_HPP

#include <core.hpp>

#include <buffer.hpp>
#include <bufferpool.hpp>

enum IoEvent {
	read,
	write,
	initialRead
};

class IoContext {
public:
    IoContext(Buffer *buffer, SOCKET connection);
    ~IoContext();

	void AddRef();

	void Release();

	long m_refCount;

    WSAOVERLAPPED m_Overlapped;
    Buffer *m_buffer;
    DWORD m_nSent;
    DWORD m_nTotal;
	DWORD m_flags;
    SOCKET m_connection;
	IoEvent m_ioEvent;
	uint8_t m_opcode;
};

#endif //WEBSOCKIOCP_IOCONTEXT_HPP