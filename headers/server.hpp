#ifndef WEBSOCKSERVER_SERVER_HPP
#define WEBSOCKSERVER_SERVER_HPP

#include <core.hpp>

#include <socket.hpp>
#include <bufferpool.hpp>
#include <iocp.hpp>
#include <iocontext.hpp>
#include <threadpool.hpp>
#include <connection.hpp>

class Server {
public:
    Server(
      const char *port,
      const char *address,
      int maxSocketNum,
      int maxThreadCount,
      int maxBufNum);

    bool Setup();

    void Run();
private:
    Socket m_listenSocket;
    IoCPort m_iocPort;
    Threadpool m_threadpool;
	Bufferpool m_bufferpool;

    CRITICAL_SECTION m_criticalSection;

    std::vector<Connection*> m_connections;

    const char *m_port;
    const char *m_address;

    int m_maxBufNum;
    int m_maxSocketNum;
    int m_maxThreadCount;
};

#endif //WEBSOCKSERVER_SERVER_HPP
