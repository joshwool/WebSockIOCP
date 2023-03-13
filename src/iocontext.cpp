#include <iocontext.hpp>

IoContext::IoContext(Buffer *buffer)
	:
		m_nTotal(0),
		m_Overlapped({}),
		m_wsabuf({}),
		m_nSent(0),
		m_connection(INVALID_SOCKET),
		m_buffer(buffer),
		m_flags(0) {
    m_wsabuf = *m_buffer->GetWSABUF();
}

IoContext::~IoContext() {
	m_buffer->m_parentPool->BufferPlace(m_buffer);
}