#ifndef WEBSOCKIOCP_THREADPOOL_HPP
#define WEBSOCKIOCP_THREADPOOL_HPP

#include <core.hpp>

#include <thread.hpp>

class Threadpool {
public:
    Threadpool(int threadCount, HANDLE iocPort);
    ~Threadpool();
private:
    std::vector<Thread*> m_threadVec; // Vector of threads
    int m_threadCount; // Thread count
};

#endif //WEBSOCKIOCP_THREADPOOL_HPP
