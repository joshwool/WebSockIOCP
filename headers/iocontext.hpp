#ifndef WEBSOCKIOCP_IOCONTEXT_HPP
#define WEBSOCKIOCP_IOCONTEXT_HPP

#include <core.hpp>

class Server;

class IOContext {
public:
    IOContext(Server *server);
    ~IOContext();


    WSAOVERLAPPED m_Overlapped;
    Buffer *m_buffer;
    WSABUF m_wsabuf;
    int m_nSent;
    int m_nTotal;
    SOCKET m_connection;
};

#endif //WEBSOCKIOCP_IOCONTEXT_HPP