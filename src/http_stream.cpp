#include "http_stream.hpp"
using namespace io;
http_stream::http_stream(http_handle_t handle) : m_buffer_start(0),m_buffer_size(0),m_client(handle),m_position(0) {
}
stream_caps http_stream::caps() const {
    stream_caps result;
    result.read = 1;
    result.seek = 0;
    result.write = 0;
    return result;
}
bool http_stream::ensure_buffer() {
    if(m_buffer_start>=m_buffer_size) {
        m_buffer_size = 0;
        m_buffer_start = 0;
        int result = http_read(m_client,m_buffer,sizeof(m_buffer));
        if(result<=0) {
            return false;
        }
        m_buffer_size = (size_t)result;
    }
    return true;
}
void* http_stream::handle() const {
    return m_client;
}
void http_stream::set(http_handle_t handle) {
    m_client = handle;
    m_buffer_size = 0;
    m_buffer_start = 0;
}
int http_stream::getch() {
    ensure_buffer();
    if(m_buffer_start>=m_buffer_size) {
        return -1;
    }
    ++m_position;
    return m_buffer[m_buffer_start++];      
}
size_t http_stream::read(uint8_t* data, size_t size) {
    size_t result = 0;
    while(size>0) {
        ensure_buffer();
        if(m_buffer_start>=m_buffer_size) {
            return result;
        }
        size_t to_read = size<m_buffer_size?size:m_buffer_size;
        memcpy(data,m_buffer,to_read);
        data+=to_read;
        size-=to_read;
        result += to_read;
        m_position+=to_read;
    }
    return result;
}
int http_stream::putch(int value) {
    return 0;
}
size_t http_stream::write(const uint8_t* data,size_t size) {
    return 0;
}
unsigned long long http_stream::seek(long long position, seek_origin origin) {
    return m_position;
}