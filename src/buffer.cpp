#include <buffer.hpp>

Buffer::Buffer(size_t bufSize, Bufferpool *parentPool)
		:
		m_totalSize(bufSize),
		m_parentPool(parentPool) {
	m_buffer = (char*)malloc(m_totalSize);

	SetupWSABUF();
}

Buffer::~Buffer() {
	delete m_buffer;
}

void Buffer::SetupWSABUF() {
    m_wsabuf.buf = m_buffer;
    m_wsabuf.len = m_totalSize;
}

void Buffer::AddData(const char* pData, size_t dataLength) {
	memcpy(m_wsabuf.buf, pData, dataLength);
	m_wsabuf.len = dataLength;
}

void Buffer::ClearBuf() {
	memset(m_buffer, 0, m_totalSize);
	m_wsabuf.len = m_totalSize;
}

char *Buffer::GetBuffer() {
    return m_buffer;
}

WSABUF *Buffer::GetWSABUF() {
    return &m_wsabuf;
}

Bufferpool *Buffer::GetParentPool() {
	return m_parentPool;
}