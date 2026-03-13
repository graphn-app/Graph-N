
#pragma once

namespace blob
{

///////////////   Writer
//////////////////////////////

class Blob final
{
public:
    Blob() = default;
    Blob(const void* buffer, size_t len)
    {
        m_blob.resize(len);
        memcpy(m_blob.data(), buffer, len);
    }
    ~Blob() = default;

    Blob(const Blob&) = delete;
    Blob& operator=(const Blob&) = delete;

    inline Blob(Blob&& other) noexcept
        : m_blob(std::move(other.m_blob))
    { }
    inline Blob& operator=(Blob&& other) noexcept
    {
        if (this != &other) m_blob = std::move(other.m_blob);
        return *this;
    }

    inline Blob(byte_buff&& buffer) noexcept
        : m_blob(std::move(buffer))
    { }
    inline Blob& operator=(byte_buff&& buffer) noexcept
    {
        m_blob = std::move(buffer);
        return *this;
    }

    inline bool operator==(const Blob& other) const
    {
        if (m_blob.size() != other.m_blob.size())
            return false;
        return memcmp(m_blob.data(), other.m_blob.data(), m_blob.size()) == 0;
    }
    inline bool operator!=(const Blob& other) const
    {
        return !(*this == other);
    }

    inline byte_buff* operator->() { return &m_blob; }
    inline const byte_buff* operator->() const { return &m_blob; }

    inline byte_buff& operator*() { return m_blob; }
    inline const byte_buff& operator*() const { return m_blob; }

private:
    byte_buff m_blob;
};

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
    //std::string pop_string();
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
