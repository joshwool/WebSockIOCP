#ifndef WEBSOCKIOCP_BUFFER_HPP
#define WEBSOCKIOCP_BUFFER_HPP

#include <core.hpp>

class Bufferpool;

class Buffer {
public:
    void SetupWSABUF(); // Sets up buffer for use in socket IO

	Buffer(size_t bufSize, Bufferpool *parentPool);
	~Buffer();

	void AddData(const char* pData, size_t dataLength); // Adds data to the buffer

    void ClearBuf(); // Clears the buffer of all contents

    WSABUF *GetWSABUF();

	Bufferpool *GetParentPool();
private:
	Bufferpool *m_parentPool;

	size_t m_usedSize;
    size_t m_totalSize;
	char *m_buffer;
    WSABUF m_wsabuf;
};

#endif //WEBSOCKIOCP_BUFFER_HPP
