#pragma once
#include "io_stream.hpp"
#include <stddef.h>
#include <stdint.h>
#include "http.h"
class http_stream : public io::stream {
    uint8_t m_buffer[1024];
    size_t m_buffer_start;
    size_t m_buffer_size;
    http_handle_t m_client;
    unsigned long long m_position;
    bool ensure_buffer();
public:
    http_stream(http_handle_t handle);
    virtual io::stream_caps caps() const override;
    virtual int getch() override;
    virtual size_t read(uint8_t* data, size_t size) override;
    virtual int putch(int value) override;
    virtual size_t write(const uint8_t* data, size_t size) override;
    virtual unsigned long long seek(long long position, io::seek_origin origin) override;
};