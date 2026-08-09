#pragma once
#include "core/base/structs.h"

namespace blob
{

///////////////   Writer
//////////////////////////////

class Writer final
{
public:
    Writer() noexcept;
    ~Writer() noexcept;

    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;
    Writer(Writer&& other) noexcept;
    Writer& operator=(Writer&& other) noexcept;

    const Blob& get() const noexcept { return m_buffer; }
    Blob release() noexcept { return std::move(m_buffer); }
    void reset() noexcept { m_buffer->clear(); }

    void patch(size_t offset, uint32_t val) noexcept;

    Writer& push(bool val);
    Writer& push(uint8_t val);
    Writer& push(int32_t val);
    Writer& push(uint32_t val);
    Writer& push(int64_t val);
    Writer& push(uint64_t val);
    Writer& push(float val);
    Writer& push(double val);
    Writer& push(number val);
    Writer& push(const byte* data, size_t len);
    Writer& push(const float* val, size_t len);
    Writer& push(const byte_buff& data);
    Writer& push(const char* val, size_t len);
    Writer& push(const std::string& val);
    Writer& push(const Primitive& val);
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
    explicit Reader(const Blob& buffer) noexcept;
    ~Reader() noexcept;

    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&& other) noexcept;
    Reader& operator=(Reader&& other) noexcept;

    bool empty() const noexcept;

    Reader clone() const noexcept;

    bool pop_bool() noexcept;
    uint8_t pop_u8() noexcept;
    int32_t pop_i32() noexcept;
    uint32_t pop_u32() noexcept;
    int64_t pop_i64() noexcept;
    uint64_t pop_u64() noexcept;
    float pop_float() noexcept;
    double pop_double() noexcept;
    number pop_number() noexcept;
    void pop_buff(float* arr) noexcept;
    void pop_buff(byte* data) noexcept;
    byte_buff pop_buff();
    std::string pop_string();
    std::string_view pop_string_ptr() noexcept;
    Primitive pop_primitive();
    ImVec2 pop_vec2() noexcept;
    ImVec4 pop_vec4() noexcept;
    ImRect pop_rect() noexcept;
    ImU32 pop_clr() noexcept;
    iid pop_iid() noexcept;
    wid pop_wid() noexcept;
    hid pop_hid() noexcept;
    Blob pop_blob();

    Reader pop_block() noexcept;

private:
    Reader(const byte* data, size_t size) noexcept;
    bool test_len(size_t len) const noexcept;

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
