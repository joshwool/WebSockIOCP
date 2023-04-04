#include "iocp.hpp"

IoCPort::IoCPort(int maxConcThreads) {
    m_handle = CreateIoCompletionPort( // Create an IO completion port
            INVALID_HANDLE_VALUE,
            nullptr,
            0,
            maxConcThreads);

    if (m_handle == nullptr) {
        std::cerr << "CreateIoCompletionPort() failed: " << GetLastError() << std::endl;
    }
}

IoCPort::~IoCPort() {
    ClosePort();
    PostCompletionPacket(0); // Posts a 0 packet on port
}

void IoCPort::ClosePort() {
    CloseHandle(m_handle); // Closes the IoCPort
}

bool IoCPort::AssignSocket(SOCKET socket, ULONG_PTR socketContext) { // Assigns a new connection to the IO Completion port
    m_handle = CreateIoCompletionPort(
            (HANDLE)socket,
            m_handle,
            socketContext,
            0);

    if (m_handle == nullptr) {
        std::cerr << "CreateIoCompletionPort() failed: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}

void IoCPort::PostCompletionPacket(ULONG_PTR completionKey) { // Manually post a completion packet
    if (PostQueuedCompletionStatus(
            m_handle,
            sizeof(completionKey),
            completionKey,
            nullptr) == 0) {
        std::cerr << "PostQueuedCompletionStatus() failed: " << GetLastError() << std::endl;
    }
}

HANDLE IoCPort::GetHandle() {
    return m_handle;
}