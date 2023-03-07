#include <iocontext.hpp>

IOContext::IOContext(Server *server) {
    m_buffer = server->BufPop();
    m_wsabuf = *m_buffer->GetWSABUF();
}