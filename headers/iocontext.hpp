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

class Connection;

class IoContext {
public:
    IoContext(Buffer *buffer, std::shared_ptr<Connection> connection);
    ~IoContext();

    WSAOVERLAPPED m_Overlapped;
    Buffer *m_buffer;
    WSABUF m_wsabuf;
    DWORD m_nSent;
    DWORD m_nTotal;
	DWORD m_flags;
    std::shared_ptr<Connection> m_pConnection;
	IoEvent m_ioEvent;
};

#endif //WEBSOCKIOCP_IOCONTEXT_HPP