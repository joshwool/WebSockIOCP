//
// Created by joshi on 07/03/2023.
//

#include <connection.hpp>

Connection::Connection() : m_handle(INVALID_SOCKET) {

}

Connection::~Connection() {
    if (m_handle != INVALID_SOCKET) {
        closesocket(m_handle);
    }
}

SOCKET Connection::GetHandle() {
    return m_handle;
}