//
// Created by joshi on 06/03/2023.
//

#ifndef WEBSOCKIOCP_BUFFER_HPP
#define WEBSOCKIOCP_BUFFER_HPP

#include <core.hpp>

class Buffer {
public:
    Buffer(size_t bufSize);
    ~Buffer();

    void SetupWSABUF();

    void ReAllocMem(size_t newSize);

    void ClearMem();

    char *GetBuffer();

    WSABUF *GetWSABUF();
private:
    char *m_buffer;
    size_t m_totalSize;
    size_t m_usedSize;

    WSABUF m_wsabuf;
};

#endif //WEBSOCKIOCP_BUFFER_HPP
