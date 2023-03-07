#include <buffer.hpp>

Buffer::Buffer(size_t bufSize) : m_totalSize(bufSize) {
    m_buffer = (char*)malloc(bufSize);

    SetupWSABUF();
}

void Buffer::SetupWSABUF() {
    m_wsabuf.buf = m_buffer;
    m_wsabuf.len = m_totalSize;
}

void Buffer::ReAllocMem(size_t newSize) {
    m_buffer = (char*)realloc(m_buffer, newSize);

    if (!m_buffer) {
        std::cout << "realloc() failed: " << std::endl;
    }
    else {
        m_totalSize = newSize;
    }
}

Buffer::~Buffer() {
    delete[] m_buffer;
}

char *Buffer::GetBuffer() {
    return m_buffer;
}

WSABUF *Buffer::GetWSABUF() {
    return &m_wsabuf;
}