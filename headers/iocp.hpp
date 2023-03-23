#ifndef WEBSOCKIOCP_IOCP_HPP
#define WEBSOCKIOCP_IOCP_HPP

#include <core.hpp>

class IoCPort {
public:
    IoCPort(int maxConcThreads);
    ~IoCPort();
    void ClosePort(); // Closes the Port

    bool AssignSocket(SOCKET socket, ULONG_PTR socketContext); // This function assigns a new connection with the IO Completion Port

    void PostCompletionPacket(ULONG_PTR completionKey); // This function allows me to send my own completion packets through to the threads

    HANDLE GetHandle(); // Returns handle of the IOCP
private:
    HANDLE m_handle;
};

#endif //WEBSOCKIOCP_IOCP_HPP
