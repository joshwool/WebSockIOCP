#include "iocontext.hpp"

IoContext::IoContext(Buffer *buffer) {
    m_buffer = buffer;
    m_wsabuf = *m_buffer->GetWSABUF();
}