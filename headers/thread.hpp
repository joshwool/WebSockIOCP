#ifndef WEBSOCKIOCP_THREAD_HPP
#define WEBSOCKIOCP_THREAD_HPP

#include <core.hpp>

#include <iocontext.hpp>

class Threadpool;

class Thread {
public:
    Thread(HANDLE iocPort);
    ~Thread();

    friend class Threadpool;

    static DWORD WINAPI IoWork(LPVOID lpParam);

	static std::string KeyCalc(std::string key);

    bool Terminate();
private:
    HANDLE m_handle;
    DWORD m_threadId;
};

#endif //WEBSOCKIOCP_THREAD_HPP
