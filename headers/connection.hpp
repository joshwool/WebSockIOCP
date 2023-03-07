#ifndef WEBSOCKIOCP_CONNECTION_HPP
#define WEBSOCKIOCP_CONNECTION_HPP

#include <core.hpp>
#include <socket.hpp>

class Connection {
public:
    friend class Socket;

    Connection();
    ~Connection();

    SOCKET GetHandle();
private:
    SOCKET m_handle;
    sockaddr m_addr;
    int m_addrlen;
};

#endif //WEBSOCKIOCP_CONNECTION_HPP
