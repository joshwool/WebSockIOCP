#include <iocontext.hpp>
#include <utility>

IoContext::IoContext(Buffer *buffer, std::shared_ptr<Connection> connection)
	:
		m_nTotal(0),
		m_Overlapped({}),
		m_wsabuf({}),
		m_nSent(0),
		m_pConnection(std::move(connection)),
		m_buffer(buffer),
		m_flags(0) {
    m_wsabuf = *m_buffer->GetWSABUF();
}

IoContext::~IoContext() {
	m_buffer->m_parentPool->BufferPlace(m_buffer);
}