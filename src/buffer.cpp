#include <buffer.hpp>

Buffer::Buffer(size_t bufSize, Bufferpool *parentPool) : m_totalSize(bufSize), m_parentPool(parentPool) {
    m_buffer = (char*)malloc(bufSize);

    SetupWSABUF();
}

Buffer::~Buffer() {
	delete[] m_buffer;
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
	SetupWSABUF();
}

char *Buffer::GetBuffer() {
    return m_buffer;
}

WSABUF *Buffer::GetWSABUF() {
    return &m_wsabuf;
}

