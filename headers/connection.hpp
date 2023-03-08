#ifndef WEBSOCKIOCP_CONNECTION_HPP
#define WEBSOCKIOCP_CONNECTION_HPP

#include <core.hpp>

class IOContext;

class Connection {
public:
    friend class Socket;

    Connection();
    ~Connection();

    SOCKET GetHandle();

    void CreateIOContext(Server *server);
private:
    SOCKET m_handle;

    sockaddr m_addr;
    int m_addrlen;

    std::shared_ptr<IOContext> m_pIOContext;
};

#endif //WEBSOCKIOCP_CONNECTION_HPP
