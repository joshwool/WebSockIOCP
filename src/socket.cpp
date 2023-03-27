#include <socket.hpp>

Socket::Socket() {}

Socket::~Socket() {
    CloseSocket();
}

void Socket::CloseSocket() {
    if (m_handle != INVALID_SOCKET) {
        closesocket(m_handle);
    }
}

bool Socket::Create(const char* port, const char *address) { // Resolves
    addrinfo hints = {}, *res;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int result = getaddrinfo(
            address,
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
    }
    m_address = *res->ai_addr;
    m_addrlen = (int)res->ai_addrlen;
    freeaddrinfo(res);
}

bool Socket::Bind() { // Binds socket to port and address
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

bool Socket::Listen() { // Starts listening for connections on socket
    if (Bind()) {
        if (listen(m_handle, SOMAXCONN) != 0) {
            std::cout << "listen() failed: " << WSAGetLastError() << std::endl;
            closesocket(m_handle);
            return false;
        }
        return true;
    }
}

SOCKET Socket::Accept() { // Accepts connections on socket and returns connection's socket handle
	SOCKET connection = INVALID_SOCKET;

	while (true) { // Indefinitely loops until successful connection found
		connection = WSAAccept( // Waits for connection and accepts it
		  m_handle,
		  nullptr,
		  nullptr,
		  nullptr,
		  0);

		if (connection != INVALID_SOCKET) {
			std::cout << "Connection found: " << connection << std::endl;

			return connection; // Returns socket handle of new connection
		}
		else {
			std::cout << WSAGetLastError() << std::endl;
		}
	}
}

SOCKET Socket::GetHandle() {
    return m_handle;
}