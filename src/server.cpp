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
            SOCKET new_con = m_listenSocket.Accept();

			auto *pIoContext = new IoContext(m_bufferpool.BufferPop(), new_con);

			m_iocPort.AssignSocket(new_con, ULONG_PTR(pIoContext));

			pIoContext->m_ioEvent = initialRead;

			int result = WSARecv(
			  new_con,
			  &(pIoContext->m_wsabuf),
			  1,
			  &(pIoContext->m_nTotal),
			  &(pIoContext->m_flags),
			  &(pIoContext->m_Overlapped),
			  nullptr);

			if (result != 0 && WSAGetLastError() != 997) {
				std::cout << "WSARecv() failed: " << WSAGetLastError() << std::endl;
			} else {
				std::cout << pIoContext->m_nTotal << " bytes received" << std::endl;
			}
        }
    }
}
