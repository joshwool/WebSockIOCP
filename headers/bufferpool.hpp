//
// Created by joshi on 09/03/2023.
//

#ifndef WEBSOCKIOCP_BUFFERPOOL_HPP
#define WEBSOCKIOCP_BUFFERPOOL_HPP

#include <core.hpp>

#include <buffer.hpp>

class Bufferpool {
public:
	explicit Bufferpool(int maxBufNum);
	~Bufferpool();

	void BufferPlace(Buffer *buffer); // Places buffer back in pool
	Buffer *BufferPop(); // Pops a buffer out of the pool and returns it.

	int BufferCount(); // Returns the number of buffers in the pool
private:
	std::vector<Buffer*> m_buffers; // Vector of pointers to buffers, effectively the bufferpool
};

#endif // WEBSOCKIOCP_BUFFERPOOL_HPP
