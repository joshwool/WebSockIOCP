#ifndef WEBSOCKIOCP_THREADPOOL_HPP
#define WEBSOCKIOCP_THREADPOOL_HPP

#include <core.hpp>

#include <thread.hpp>

class Threadpool {
public:
    Threadpool(int threadCount, HANDLE iocPort);
    ~Threadpool();

    HANDLE *GetHandleArray();
private:
    std::vector<Thread*> m_threadVec;
    int m_threadCount;
};

#endif //WEBSOCKIOCP_THREADPOOL_HPP
