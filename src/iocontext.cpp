#include <iocontext.hpp>

IoContext::IoContext(Buffer *buffer) {
    m_buffer = buffer;
    m_wsabuf = *m_buffer->GetWSABUF();
}

IoContext::~IoContext() {
	m_buffer->m_parentPool->BufferPlace(m_buffer);
}