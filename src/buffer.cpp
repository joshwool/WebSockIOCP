#include <buffer.hpp>

Buffer::Buffer(size_t bufSize, Bufferpool *parentPool)
		:
		m_totalSize(bufSize),
		m_parentPool(parentPool) {
	m_buffer = (char*)malloc(m_totalSize); // Allocates memory to the buffer

	SetupWSABUF(); // Setups the WSABUF structure with the buffer created above
}

Buffer::~Buffer() { // Deletes buffer when destructor is called
	delete m_buffer;
}

void Buffer::SetupWSABUF() {
    m_wsabuf.buf = m_buffer; // Sets WSABUF buffer to member buffer
    m_wsabuf.len = m_totalSize; // Sets WSABUF length to size of member BUFFER
}

void Buffer::AddData(const char* pData, size_t dataLength) {
	memcpy(m_wsabuf.buf, pData, dataLength); // Copies data to the buffer
	m_wsabuf.len = dataLength; // Sets the buffer length to the data length for send
}

void Buffer::ClearBuf() {
	memset(m_buffer, 0, m_totalSize); // Sets all bytes in buffer to 0, effectively clearing it
	m_wsabuf.len = m_totalSize; // Sets size of readable part to default
}

WSABUF *Buffer::GetWSABUF() {
    return &m_wsabuf; // Returns a pointer to the buffer for socket IO
}

Bufferpool *Buffer::GetParentPool() {
	return m_parentPool; // Returns a pointer to this buffer's bufferpool
}