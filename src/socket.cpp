//
// Created by joshi on 24/02/2023.
//

#include <socket.hpp>

Socket::Socket() : m_handle(INVALID_SOCKET) {}

Socket::~Socket() {
    CloseSocket();
}

void Socket::CloseSocket() {
    if (m_handle != INVALID_SOCKET) {
        closesocket(m_handle);
    }
}

bool Socket::Create(const char* port) {
    addrinfo hints, *res;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int result = getaddrinfo(
            nullptr,
            port,
            &hints,
            &res);

    if (result != 0) {
        std::cout << "getaddrinfo() failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    m_handle = WSASocketW(
            res->ai_family,
            res->ai_socktype,
            res->ai_protocol,
            nullptr,
            0,
            WSA_FLAG_OVERLAPPED);

    if (m_handle == INVALID_SOCKET) {
        std::cout << "socket() failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }
    m_address = *res->ai_addr;
    m_addrlen = res->ai_addrlen;
    freeaddrinfo(res);
    return true;
}

bool Socket::Bind() {
    int result = bind(
            m_handle,
            &m_address,
            m_addrlen);

    if (result != 0) {
        std::cout << "bind() failed: " << WSAGetLastError() << std::endl;
        closesocket(m_handle);
        return false;
    }
    return true;
}

bool Socket::Listen() {
    if (Bind()) {
        if (listen(m_handle, SOMAXCONN) != 0) {
            std::cout << "listen() failed: " << WSAGetLastError() << std::endl;
            closesocket(m_handle);
            return false;
        }
        return true;
    }
}

Connection *Socket::Accept() {
    Connection *connection = new Connection;
    connection->m_handle = accept(
            m_handle,
            &connection->m_addr,
            &connection->m_addrlen);

    return connection;
}

SOCKET Socket::GetHandle() {
    return m_handle;
}