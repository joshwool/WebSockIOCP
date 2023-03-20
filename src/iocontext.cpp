#include <iocontext.hpp>
#include <utility>

IoContext::IoContext(Buffer *buffer, SOCKET connection)
	:
		m_refCount(1),
		m_nTotal(0),
		m_Overlapped({}),
		m_wsabuf({}),
		m_nSent(0),
		m_connection(connection),
		m_buffer(buffer),
		m_flags(0) {
    m_wsabuf = *m_buffer->GetWSABUF();
}

IoContext::~IoContext() {
	std::cout << "Buf Returned" << std::endl;
	m_buffer->ClearBuf();
	std::cout << m_buffer->GetBuffer() << std::endl;
	m_buffer->m_parentPool->BufferPlace(m_buffer);
}

void IoContext::AddRef() {
	InterlockedIncrement(&m_refCount);
}

void IoContext::Release() {
	InterlockedDecrement(&m_refCount);

	if (m_refCount == 0) {
		std::cout << "Connection Closed: " << m_connection << std::endl;
		delete this;
	}
}