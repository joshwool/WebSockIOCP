#ifndef WEBSOCKIOCP_THREAD_HPP
#define WEBSOCKIOCP_THREAD_HPP

#include <core.hpp>
#include <iocontext.hpp>

class Threadpool;

class Thread {
public:
    Thread(HANDLE iocPort, Threadpool *parentPool);
    ~Thread();

    friend class Threadpool;

    static DWORD WINAPI IoWork(LPVOID lpParam);
private:
    Threadpool *parentPool;
    HANDLE handle;
    DWORD threadId;
};

#endif //WEBSOCKIOCP_THREAD_HPP
