#include <threadpool.hpp>

Threadpool::Threadpool(int threadCount, HANDLE iocPort) : m_threadCount(threadCount) {
    for (int i = 0; i < threadCount; i++) {
        m_threadVec.push_back(new Thread(iocPort)); // Creates new threads and adds it to threadpool
    }
}

Threadpool::~Threadpool() {
	for (Thread *thread : m_threadVec) { // Deletes all threads in threadpool
		delete thread;
	}
}

