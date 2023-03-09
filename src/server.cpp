#include <server.hpp>

Server::Server(const char *port, const char *address, int maxSocketNum, int maxThreadCount, int maxBufNum)
    :
      m_port(port),
      m_address(address),
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
    }

    InitializeCriticalSection(&m_criticalSection);
}

Buffer *Server::BufPop() {
    Buffer *buf = m_bufs.back();
    m_bufs.pop_back();
    return buf;
}

bool Server::Setup() {
    if (m_listenSocket.Create(m_port, m_address)) {
    }
    else {
        return false;
    }
}

void Server::Run() {
    if (m_listenSocket.Listen()) {
        while (true) {
            Connection *new_con = m_listenSocket.Accept();
            m_connections.push_back(new_con);

            EnterCriticalSection(&m_criticalSection);

            new_con->SetpIoContext(std::make_shared<IoContext>(this->BufPop()));

            m_iocPort.AssignSocket(new_con->GetHandle(), ULONG_PTR(new_con->GetpIoContext()));

            m_iocPort.PostCompletionPacket(0);
        }
    }
}

