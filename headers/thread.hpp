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

    bool Terminate();
private:
    Threadpool *m_parentPool;

    HANDLE m_handle;
    DWORD m_threadId;
};

#endif //WEBSOCKIOCP_THREAD_HPP
