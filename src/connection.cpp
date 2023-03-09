#include <connection.hpp>

Connection::Connection() : m_handle(INVALID_SOCKET) {}

Connection::~Connection() {
    if (m_handle != INVALID_SOCKET) {
        closesocket(m_handle);
    }
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