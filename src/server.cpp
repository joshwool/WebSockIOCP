#include <server.hpp>

Server::Server(const char* port, int maxSocketNum, int maxThreadCount, int maxBufNum) :
        port(port),
        m_maxSocketNum(maxSocketNum),
        m_maxThreadCount(maxThreadCount),
        m_maxBufNum(maxBufNum),
        m_iocPort(maxThreadCount),
        m_threadpool(maxThreadCount, m_iocPort.GetHandle()) {
    for (int i = 0; i < maxBufNum; i++) {
        m_bufs.at(i) = new Buffer(MAX_BUF_MEM);

        if (!m_bufs.at(i)) {
            std::cout << "Buffer " << i << " not created" << std::endl;
        }

        m_bufs.at(i)->SetupWSABUF();
    }
}

Buffer *Server::BufPop() {
    Buffer *buf = m_bufs.back();
    m_bufs.pop_back();
    return buf;
}

bool Server::Setup() {
    m_listenSocket.Create(port);


}

void Server::Run() {
    while (true) {
        Connection *new_con = m_listenSocket.Accept();
        m_connections.push_back(new_con);

         IOContext *ioContext = new IOContext(this);




    }
}

