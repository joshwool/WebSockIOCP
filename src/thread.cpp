#include <thread.hpp>

Thread::Thread(HANDLE iocPort, Threadpool *parentPool) : handle(INVALID_HANDLE_VALUE), parentPool(parentPool) {
    handle = CreateThread(
            nullptr,
            0,
            Thread::IoWork,
            iocPort,
            0,
            &threadId);

    if (handle == INVALID_HANDLE_VALUE) {
        std::cout << "CreateThread() failed: " << GetLastError() << std::endl;
    }
}

Thread::~Thread() {
    CloseHandle(handle);
}

DWORD WINAPI Thread::IoWork(LPVOID IoContext) {
    HANDLE iocpHandle = (HANDLE)IoContext;
    DWORD totalBytes;
    IOContext *ioContext;
    LPWSAOVERLAPPED overlapped;

    while (true) {
        if (GetQueuedCompletionStatus(
                iocpHandle,
                &totalBytes,
                (PULONG_PTR)&ioContext,
                &overlapped,
                INFINITE)) {

        }
    }
}