#include <threadpool.hpp>

Threadpool::Threadpool(int threadCount, HANDLE iocPort) : m_threadCount(threadCount) {
    for (int i = 0; i < threadCount; i++) {
        m_threadVec.push_back(new Thread(iocPort, this));
    }
}

HANDLE *Threadpool::GetHandleArray() {
    HANDLE *handleArr = (HANDLE *)malloc(sizeof(HANDLE) * m_threadCount);

    for (int i = 0; i < m_threadCount; i++) {
        handleArr[i] = m_threadVec[i]->handle;
    }

    return handleArr;
}