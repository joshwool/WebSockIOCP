#ifndef WEBSOCKIOCP_IOCONTEXT_HPP
#define WEBSOCKIOCP_IOCONTEXT_HPP

#include <core.hpp>
#include <buffer.hpp>

class IoContext {
public:
    IoContext(Buffer *buffer);
    ~IoContext();


    WSAOVERLAPPED m_Overlapped;
    Buffer *m_buffer;
    WSABUF m_wsabuf;
    int m_nSent;
    int m_nTotal;
    SOCKET m_connection;
};

#endif //WEBSOCKIOCP_IOCONTEXT_HPP