#ifndef WEBSOCKSERVER_SERVER_HPP
#define WEBSOCKSERVER_SERVER_HPP

#include <core.hpp>

#include <buffer.hpp>
#include <connection.hpp>

class Server {
public:
    Server(const char* port,
           int maxSocketNum,
           int maxThreadCount,
           int maxBufNum);

    bool Setup();

    void Run();

    Buffer *BufPop();

private:
    Socket m_listenSocket;
    IoCPort m_iocPort;
    Threadpool m_threadpool;

    std::vector<Buffer*> m_bufs;

    std::vector<Connection*> m_connections;

    const char* port;

    int m_maxBufNum;
    int m_maxSocketNum;
    int m_maxThreadCount;
};

#endif //WEBSOCKSERVER_SERVER_HPP
