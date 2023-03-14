#include <connection.hpp>

Connection::Connection() : m_handle(INVALID_SOCKET) {}

Connection::~Connection() {
    if (m_handle != INVALID_SOCKET) {
        closesocket(m_handle);
    }
}

void Connection::InitialRead() {
	m_pIoContext->m_ioEvent = initialRead;

	int result = WSARecv(
	  m_handle,
	  &(m_pIoContext->m_wsabuf),
	  1,
	  &(m_pIoContext->m_nTotal),
	  &(m_pIoContext->m_flags),
	  &(m_pIoContext->m_Overlapped),
	  nullptr);

	if (result != 0 && WSAGetLastError() != 997) {
		std::cout << "IntialRead() failed: " << WSAGetLastError() << std::endl;
	}
	else {
		std::cout << m_pIoContext->m_nTotal << " bytes received" << std::endl;
	}
}

void Connection::Read() {
	m_pIoContext->m_ioEvent = read;

	int result = WSARecv(
	  m_handle,
	  &(m_pIoContext->m_wsabuf),
	  1,
	  &(m_pIoContext->m_nTotal),
	  &(m_pIoContext->m_flags),
	  &(m_pIoContext->m_Overlapped),
	  nullptr);

	if (result != 0 && WSAGetLastError() != 997) {
		std::cout << "IntialRead() failed: " << WSAGetLastError() << std::endl;
	}
	else {
		std::cout << m_pIoContext->m_nTotal << " bytes received" << std::endl;
	}
}

void Connection::Write() {
	m_pIoContext->m_ioEvent = write;
}


SOCKET Connection::GetHandle() {
    return m_handle;
}

void Connection::SetpIoContext(std::shared_ptr<IoContext> pIoContext) {
    m_pIoContext = pIoContext;
}

IoContext *Connection::GetpIoContext() {
    return m_pIoContext.get();
}