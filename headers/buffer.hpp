#ifndef WEBSOCKIOCP_BUFFER_HPP
#define WEBSOCKIOCP_BUFFER_HPP

#include <core.hpp>

class Bufferpool;

class Buffer {
public:
	friend class IoContext;

    Buffer(size_t bufSize, Bufferpool *parentPool);
    ~Buffer();

    void SetupWSABUF();

    void ReAllocMem(size_t newSize);

    void ClearMem();

    char *GetBuffer();

    WSABUF *GetWSABUF();
private:
	Bufferpool *m_parentPool;

    char *m_buffer;
    size_t m_totalSize;
    size_t m_usedSize;

    WSABUF m_wsabuf;
};

#endif //WEBSOCKIOCP_BUFFER_HPP
