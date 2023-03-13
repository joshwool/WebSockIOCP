#include "iocp.hpp"

IoCPort::IoCPort(int maxConcThreads) {
    m_handle = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,
            nullptr,
            0,
            maxConcThreads);

    if (m_handle == nullptr) {
        std::cout << "CreateIoCompletionPort() failed: " << GetLastError() << std::endl;
    }
}

IoCPort::~IoCPort() {
    ClosePort();
    PostCompletionPacket(0);
}

void IoCPort::ClosePort() {
    CloseHandle(m_handle);
}

bool IoCPort::AssignSocket(SOCKET socket, ULONG_PTR socketContext) {
    m_handle = CreateIoCompletionPort(
            (HANDLE)socket,
            m_handle,
            socketContext,
            0);

    if (m_handle == nullptr) {
        std::cout << "CreateIoCompletionPort() failed: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}

void IoCPort::PostCompletionPacket(ULONG_PTR completionKey) {
    if (PostQueuedCompletionStatus(
            m_handle,
            sizeof(completionKey),
            completionKey,
            nullptr) == 0) {
        std::cout << "PostQueuedCompletionStatus() failed: " << GetLastError() << std::endl;
    }
}

HANDLE IoCPort::GetHandle() {
    return m_handle;
}