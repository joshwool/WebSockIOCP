//
// Created by joshi on 24/02/2023.
//

#ifndef WEBSOCKIOCP_SOCKET_HPP
#define WEBSOCKIOCP_SOCKET_HPP

#include <core.hpp>
#include <iocp.hpp>
#include <connection.hpp>

class Socket {
public:
    Socket();
    ~Socket();
    void CloseSocket();

    bool Create(const char* port);

    bool Bind();
    bool Listen();

    Connection *Accept();

    SOCKET GetHandle();
private:
    sockaddr m_address;
    size_t m_addrlen;
    SOCKET m_handle;
};

#endif //WEBSOCKIOCP_SOCKET_HPP
