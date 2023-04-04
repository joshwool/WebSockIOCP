#ifndef WEBSOCKSERVER_SERVER_HPP
#define WEBSOCKSERVER_SERVER_HPP

#include <core.hpp>

#include <socket.hpp>
#include <bufferpool.hpp>
#include <iocp.hpp>
#include <iocontext.hpp>
#include <threadpool.hpp>
#include <database.hpp>

class Server {
public:
	Server(const char *port, const char *address, char *db_dir, int maxThreadCount, int maxBufNum);

    bool Setup(); // Setups the server

    void Run(); // Runs the server

private:
    Socket m_listenSocket;
    IoCPort m_iocPort;
    Threadpool m_threadpool;
	Bufferpool m_bufferpool;
	Database m_db;

    const char *m_port;
    const char *m_address;
};

#endif //WEBSOCKSERVER_SERVER_HPP
