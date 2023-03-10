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

	void BufferPlace(Buffer *buffer);
	Buffer *BufferPop();
private:
	std::vector<Buffer*> m_buffers;
};

#endif // WEBSOCKIOCP_BUFFERPOOL_HPP
