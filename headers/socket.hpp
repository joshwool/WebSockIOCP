#ifndef WEBSOCKIOCP_SOCKET_HPP
#define WEBSOCKIOCP_SOCKET_HPP

#include <core.hpp>

class Socket {
public:
    Socket();
    ~Socket();
    void CloseSocket(); // Closes the socket

    bool Create(const char* port, const char *address); // Resolves address and creates a socket

    bool Bind(); // Binds socket to address and port
    bool Listen(); // Listens for new connections

    SOCKET Accept(); // Accepts new connections

private:
    sockaddr m_address;
    int m_addrlen;
    SOCKET m_handle;
};

#endif //WEBSOCKIOCP_SOCKET_HPP
