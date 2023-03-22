#include <iocontext.hpp>
#include <utility>

IoContext::IoContext(Buffer *buffer, SOCKET connection)
	:
		m_refCount(1),
		m_nTotal(0),
		m_Overlapped({}),
		m_nSent(0),
		m_connection(connection),
		m_buffer(buffer),
		m_flags(0) {
}

IoContext::~IoContext() {
	std::cout << "Buf Returned" << std::endl;
	closesocket(m_connection); // Closes socket as connection has closed
	m_buffer->ClearBuf(); // Clears buffer before returning it to the pool.
	m_buffer->GetParentPool()->BufferPlace(m_buffer); // Recycling buffers instead of destroying them reduces overhead.
}

void IoContext::AddRef() {
	InterlockedIncrement(&m_refCount); // Thread safe ref count increment
}

void IoContext::Release() {
	InterlockedDecrement(&m_refCount); // Thread safe ref count decrement

	if (m_refCount == 0) { // If ref count hits 0 connection is closed and all work has been done.
		std::cout << "Connection Closed: " << m_connection << std::endl;
		delete this; // Calls destructor
	}
}