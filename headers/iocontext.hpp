#ifndef WEBSOCKIOCP_IOCONTEXT_HPP
#define WEBSOCKIOCP_IOCONTEXT_HPP

#include <core.hpp>

#include <buffer.hpp>
#include <bufferpool.hpp>

class IoContext {
public:
    IoContext(Buffer *buffer);
    ~IoContext();

    WSAOVERLAPPED m_Overlapped;
    Buffer *m_buffer;
    WSABUF m_wsabuf;
    DWORD m_nSent;
    DWORD m_nTotal;
	DWORD m_flags;
    SOCKET m_connection;
};

#endif //WEBSOCKIOCP_IOCONTEXT_HPP