#ifndef WEBSOCKIOCP_CONNECTION_HPP
#define WEBSOCKIOCP_CONNECTION_HPP

#include <core.hpp>

#include <IoContext.hpp>

class Connection {
public:
    friend class Socket;

    Connection();
    ~Connection();

	void InitialRead();

	void Read();

	void Write();

    SOCKET GetHandle();

    void SetpIoContext(std::shared_ptr<IoContext> pIoContext);

    IoContext *GetpIoContext();
private:
    SOCKET m_handle;

    sockaddr m_addr;
    int m_addrlen;

    std::shared_ptr<IoContext> m_pIoContext;
};

#endif //WEBSOCKIOCP_CONNECTION_HPP
