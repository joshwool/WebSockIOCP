#ifndef WEBSOCKIOCP_IOCONTEXT_HPP
#define WEBSOCKIOCP_IOCONTEXT_HPP

#include <core.hpp>

#include <buffer.hpp>
#include <bufferpool.hpp>

enum IoEvent {
	read,
	write,
	initialRead,
	close
};

class IoContext {
public:
    IoContext(Buffer *buffer, mysqlx::Schema *database, SOCKET connection);
    ~IoContext();

	void AddRef(); // Increments Ref Count
	void Release(); // Decrements Ref Count, and calls destructor if count = 0

	long m_refCount;

    WSAOVERLAPPED m_Overlapped; // Overlapped structure passed with every IO operation
    Buffer *m_buffer; // Pointer to buffer used by connection
    DWORD m_nSent; // Number of bytes sent in IO operation
    DWORD m_nTotal; // Total number bytes receieved
	DWORD m_flags; // Flags for IO operations
    SOCKET m_connection; // Socket handle, for connection
	IoEvent m_ioEvent; // Enumeration with all possible IO events
	mysqlx::Schema *m_database; // SQL Database
};

#endif //WEBSOCKIOCP_IOCONTEXT_HPP