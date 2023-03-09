#include <thread.hpp>

Thread::Thread(HANDLE iocPort, Threadpool *parentPool) : m_handle(INVALID_HANDLE_VALUE), m_parentPool(parentPool) {
    m_handle = CreateThread(
            nullptr,
            0,
            Thread::IoWork,
            iocPort,
            0,
            &m_threadId);

    if (m_handle == INVALID_HANDLE_VALUE) {
        std::cout << "CreateThread() failed: " << GetLastError() << std::endl;
    }
}

Thread::~Thread() {
    Terminate();
}

DWORD WINAPI Thread::IoWork(LPVOID IoCPort) {
    HANDLE iocpHandle = (HANDLE)IoCPort;
    DWORD totalBytes;
    IoContext *ioContext;
    LPWSAOVERLAPPED overlapped;

    while (true) {
        if (GetQueuedCompletionStatus(
                iocpHandle,
                &totalBytes,
                (PULONG_PTR)&ioContext,
                &overlapped,
                INFINITE)) {
          std::cout << "Packet Received" << std::endl;
        }
    }
}

bool Thread::Terminate() {
    if (TerminateThread(
            m_handle,
            0) == 0) {
        std::cout << "TerminateThread() failed on thread " << m_threadId << " with: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}