#include <thread.hpp>

Thread::Thread(HANDLE iocPort) : m_handle(INVALID_HANDLE_VALUE), m_threadId(0) {
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
    auto iocpHandle = (HANDLE)IoCPort;
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
			std::cout << ioContext->m_wsabuf.buf << std::endl;
        }
		else {
			std::cout << GetLastError() << std::endl;
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