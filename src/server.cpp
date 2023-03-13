#include <server.hpp>

Server::Server(const char *port, const char *address, int maxSocketNum, int maxThreadCount, int maxBufNum)
    :
		m_port(port),
      	m_address(address),
		m_maxSocketNum(maxSocketNum),
		m_maxThreadCount(maxThreadCount),
		m_maxBufNum(maxBufNum),
      	m_iocPort(maxThreadCount),
		m_threadpool(maxThreadCount, m_iocPort.GetHandle()),
		m_bufferpool(maxBufNum) {
    InitializeCriticalSection(&m_criticalSection);
}



bool Server::Setup() {
    if (m_listenSocket.Create(m_port, m_address)) {
    }
    else {
        return false;
    }
    return true;
}

void Server::Run() {
    if (m_listenSocket.Listen()) {
        while (true) {
            Connection *new_con = m_listenSocket.Accept();
            m_connections.push_back(new_con);

			new_con->SetpIoContext(std::make_shared<IoContext>(m_bufferpool.BufferPop()));

			m_iocPort.AssignSocket(new_con->GetHandle(), ULONG_PTR(new_con->GetpIoContext()));

			new_con->InitialRead();
        }
    }
}