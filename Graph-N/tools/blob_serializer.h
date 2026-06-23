#pragma once
#include "core/base/structs.h"

namespace blob
{

///////////////   Writer
//////////////////////////////

class Writer final
{
public:
    Writer();
    ~Writer();

    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;
    Writer(Writer&& other);
    Writer& operator=(Writer&& other);

    inline const Blob& get() const { return m_buffer; }
    inline Blob release() { return std::move(m_buffer); }
    inline void reset() { m_buffer->clear(); }

    void patch(size_t offset, uint32_t val);

    Writer& push(bool val);
    Writer& push(uint8_t val);
    Writer& push(int32_t val);
    Writer& push(uint32_t val);
    Writer& push(int64_t val);
    Writer& push(uint64_t val);
    Writer& push(float val);
    Writer& push(double val);
    Writer& push(const byte* data, size_t len);
    Writer& push(const float* val, size_t len);
    Writer& push(const byte_buff& data);
    Writer& push(const char* val, size_t len);
    Writer& push(const std::string& val);
    Writer& push(const primitive& val);
    Writer& push(const ImVec2& val);
    Writer& push(const ImVec4& val);
    Writer& push(const ImRect& val);
    Writer& push(const Blob& val);
    Writer& push(const Writer& val);

private:
    Blob m_buffer;
};

///////////////   Reader
//////////////////////////////   

class Reader final
{
public:
    explicit Reader(const Blob& buffer);
    ~Reader();

    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&& other);
    Reader& operator=(Reader&& other);

    bool empty() const;

    Reader clone() const;

    bool pop_bool();
    uint8_t pop_u8();
    int32_t pop_i32();
    uint32_t pop_u32();
    int64_t pop_i64();
    uint64_t pop_u64();
    float pop_float();
    double pop_double();
    void pop_buff(float* arr);
    void pop_buff(byte* data);
    byte_buff pop_buff();
    std::string_view pop_string();
    primitive pop_primitive();
    ImVec2 pop_vec2();
    ImVec4 pop_vec4();
    ImRect pop_rect();
    ImU32 pop_clr();
    iid pop_iid();
    wid pop_wid();
    hid pop_hid();
    Blob pop_blob();

    Reader pop_block();

private:
    Reader(const byte* data, size_t size);
    void test_len(size_t len) const;

private:
    const byte* m_data = nullptr;
    size_t m_size;
    size_t m_offset;
};

///////////////   Block
//////////////////////////////

class Block final
{
public:
    Block(Writer& writer, bool begin_block = true);
    ~Block();

    NO_COPY_MOVE(Block);

    void begin();
    void end();

private:
    Writer& m_writer;
    size_t m_size_offset = 0;
    size_t m_start_offset = 0;
    bool m_opened = false;
};

} // namespace blob
