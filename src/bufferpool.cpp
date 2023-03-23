#include <bufferpool.hpp>

Bufferpool::Bufferpool(int maxBufNum) {
	for (int i = 0; i < maxBufNum; i++) {
		m_buffers.push_back(new Buffer(INIT_BUF_MEM, this)); // Creates a new buffer

		if (!m_buffers.at(i)) { // Checks if buffer created correctly
			std::cout << "Buffer " << i << " not created" << std::endl;
		}
	}
}

Bufferpool::~Bufferpool() {
	for (Buffer *buffer : m_buffers) { // Deletes all buffers currently in pool
		delete buffer;
	}
}

void Bufferpool::BufferPlace(Buffer *buffer) { // Places buffer back in pool
	m_buffers.push_back(buffer);
}

Buffer *Bufferpool::BufferPop() { // Pops a buffer out of the pool and returns it
	Buffer *buffer = m_buffers.back();
	m_buffers.pop_back();
	return buffer;
}

int Bufferpool::BufferCount() { // Returns number of buffers currently in pool
	return m_buffers.size();
}