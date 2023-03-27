#include <server.hpp>

Server::Server(const char *port, const char *address, char *db_dir, int maxThreadCount, int maxBufNum) :
		m_port(port),
      	m_address(address),
      	m_iocPort(maxThreadCount),
		m_threadpool(maxThreadCount, m_iocPort.GetHandle()),
		m_bufferpool(maxBufNum),
		m_db(db_dir) {
}



bool Server::Setup() { // Creates a socket on server's port and address
    if (m_listenSocket.Create(m_port, m_address)) {
		return true;
    }
    else {
        return false;
    }
}

void Server::Run() {
    if (m_listenSocket.Listen()) {
        while (true) { // Loops indefinitely, accepting new connections and assigning to the IO completion port
            SOCKET new_con = m_listenSocket.Accept();

			auto *pIoContext = new IoContext(m_bufferpool.BufferPop(), &m_db, new_con); // Creates a new IoContext object for the connection

			m_iocPort.AssignSocket(new_con, ULONG_PTR(pIoContext)); // Assigns the new connection to the IO completion port

			pIoContext->m_ioEvent = initialRead;

			int result = WSARecv( // Posts initial receive on socket
			  new_con,
			  pIoContext->m_buffer->GetWSABUF(),
			  1,
			  &(pIoContext->m_nTotal),
			  &(pIoContext->m_flags),
			  &(pIoContext->m_Overlapped),
			  nullptr);

			if (result != 0 && WSAGetLastError() != ERROR_IO_PENDING) { // IO Pending errors ignored as operation will complete later
				std::cout << "WSARecv() failed: " << WSAGetLastError() << std::endl;
			}
        }
    }
}
