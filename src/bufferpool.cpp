#include <bufferpool.hpp>

Bufferpool::Bufferpool(int maxBufNum) {
	for (int i = 0; i < maxBufNum; i++) {
		m_buffers[i] = new Buffer(INIT_BUF_MEM, this);

		if (!m_buffers.at(i)) {
			std::cout << "Buffer " << i << " not created" << std::endl;
		}
	}
}

Bufferpool::~Bufferpool() {
	for (Buffer *buffer : m_buffers) {
		delete buffer;
	}
}

void Bufferpool::BufferPlace(Buffer *buffer) {
	m_buffers.push_back(buffer);
}

Buffer *Bufferpool::BufferPop() {
	Buffer *buffer = m_buffers.back();
	m_buffers.pop_back();
	return buffer;
}