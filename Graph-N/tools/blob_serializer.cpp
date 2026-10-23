/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "tools/blob_serializer.h"

namespace blob
{
///////////////   primitive
//////////////////////////////

#ifndef TOOLS_DETACHED_STATE
enum PrimitiveType : uint8_t
{
    pt_none,
    pt_i64,
    pt_double,
    pt_number,
    pt_bool,
    pt_string,
    pt_monostate
};
#endif // TOOLS_DETACHED_STATE


template <typename TRIVIAL>
void push_trivial(TRIVIAL val, Blob& buffer)
{
    static_assert(std::is_trivially_copyable_v<TRIVIAL>, "Type is not safe for memcpy!");
    const byte* ptr = cast::ptrs<const byte>(&val);
    buffer->insert(buffer->end(), ptr, ptr + sizeof(TRIVIAL));
}

template <typename TRIVIAL>
TRIVIAL pop_trivial(const byte* data, size_t& offset) noexcept
{
    static_assert(std::is_trivially_copyable_v<TRIVIAL>, "Type is not safe for memcpy!");
    TRIVIAL value = {};
    std::memcpy(&value, data + offset, sizeof(TRIVIAL));
    offset += sizeof(TRIVIAL);
    return value;
}

///////////////   Writer
//////////////////////////////   

Writer::Writer() noexcept = default;

Writer::~Writer() noexcept
{
    LogAssert(m_patches.empty(), "Not all patches were closed: " << m_patches.size());
}

Writer::Writer(Writer&& other) noexcept
{
    *this = std::move(other);
}

Writer& Writer::operator=(Writer&& other) noexcept
{
    if (this == &other)
        return *this;

    m_buffer = std::move(other.m_buffer);
    return *this;
}

Blob Writer::release() noexcept
{
    LogAssert(m_patches.empty(), "Not all patches were closed: " << m_patches.size());
    return std::move(m_buffer);
}

Writer& Writer::push(bool val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(uint8_t val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(int32_t val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(uint32_t val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(int64_t val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(uint64_t val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(float val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(double val)
{
    push_trivial(val, m_buffer);
    return *this;
}

#ifndef TOOLS_DETACHED_STATE
Writer& Writer::push(number val)
{
    static_assert(std::same_as<number_t, double>, "Check this function if number_t changes");
    push_trivial(val.underlying_type(), m_buffer);
    return *this;
}
#endif // TOOLS_DETACHED_STATE

Writer& Writer::push(const byte* data, size_t len)
{
    push(cast::to_u32(len));
    if (len > 0)
        m_buffer->insert(m_buffer->end(), data, data + len);
    return *this;
}

Writer& Writer::push(const float* val, size_t len)
{
    push(cast::ptrs<byte>(val), len * sizeof(float));
    return *this;
}

Writer& Writer::push(const byte_buff& data)
{
    push(data.data(), data.size());
    return *this;
}

Writer& Writer::push(const char* val, size_t len)
{
    push(cast::ptrs<byte>(val), len);
    return *this;
}

Writer& Writer::push(const std::string& val)
{
    push(val.c_str(), val.size());
    return *this;
}

Writer& Writer::push(const std::wstring& val)
{
    push(cast::ptrs<byte>(val.data()), val.size() * sizeof(wchar_t));
    return *this;
}

#ifndef TOOLS_DETACHED_STATE
Writer& Writer::push(const Primitive& val)
{
    std::visit([this](const auto& v)
    {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, int64_t>)
        {
            push(uint8_t(pt_i64)).push(v);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            push(uint8_t(pt_double)).push(v);
        }
        else if constexpr (std::is_same_v<T, number>)
        {
            push(uint8_t(pt_number)).push(v);
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            push(uint8_t(pt_bool)).push(v);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            push(uint8_t(pt_string)).push(v);
        }
        else if constexpr (std::is_same_v<T, std::monostate>)
        {
            push(uint8_t(pt_monostate));
        }
        else
        {
            static_assert(sizeof(T) == 0, "Undefined primitive type in visitor");
        }
    }, val.value());

    return *this;
}

Writer& Writer::push(const ImVec2& val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(const ImVec4& val)
{
    push_trivial(val, m_buffer);
    return *this;
}

Writer& Writer::push(const ImRect& val)
{
    push_trivial(val, m_buffer);
    return *this;
}
#endif // TOOLS_DETACHED_STATE

Writer& Writer::push(const Blob& val)
{
    push(*val);
    return *this;
}

Writer& Writer::push(const Writer& val)
{
    push(val.m_buffer->data(), val.m_buffer->size());    
    return *this;
}

void Writer::patch_begin() noexcept
{
    const auto off = m_buffer->size();
    m_patches.push_back(off);
    push(uint32_t(0));
}

void Writer::patch_end(uint32_t val) noexcept
{
    LogRAssert(!m_patches.empty(), "No open patches", );
    auto off = m_patches.back();    
    m_patches.pop_back();
    LogRAssert(off + sizeof(uint32_t) <= m_buffer->size(), "Patch: Out of range", );
    std::memcpy(m_buffer->data() + off, &val, sizeof(uint32_t));
}

///////////////   Reader
//////////////////////////////   

Reader::Reader(const Blob& buffer) noexcept
    : m_data(buffer->data())
    , m_size(buffer->size())
    , m_offset(0)
{}

Reader::Reader(const byte* data, size_t size) noexcept
    : m_data(data)
    , m_size(size)
    , m_offset(0)
{}

Reader::~Reader() noexcept = default;

Reader::Reader(Reader&& other) noexcept
    : m_data(std::exchange(other.m_data, nullptr))
    , m_size(std::exchange(other.m_size, 0))
    , m_offset(std::exchange(other.m_offset, 0))
{}

Reader& Reader::operator=(Reader&& other) noexcept
{
    if (this == &other)
        return *this;

    m_data = std::exchange(other.m_data, nullptr);
    m_size = std::exchange(other.m_size, 0);
    m_offset = std::exchange(other.m_offset, 0);
    return *this;
}

bool Reader::empty() const noexcept
{
    return m_offset >= m_size;
}

Reader Reader::clone() const noexcept
{
    Reader clone(m_data, m_size);
    clone.m_offset = m_offset;
    return clone;
}

bool Reader::test_len(size_t len) const noexcept
{
    const bool is_valid = (len <= m_size - m_offset);
    LogRAssert(is_valid, "Reader len: Out of range", false);
    return is_valid;
}

bool Reader::pop_bool() noexcept
{
    if (!test_len(sizeof(bool))) return false;
    return pop_trivial<bool>(m_data, m_offset);
}

uint8_t Reader::pop_u8() noexcept
{
    if (!test_len(sizeof(uint8_t))) return 0;
    return pop_trivial<uint8_t>(m_data, m_offset);
}

int32_t Reader::pop_i32() noexcept
{
    if (!test_len(sizeof(int32_t))) return 0;
    return pop_trivial<int32_t>(m_data, m_offset);
}

uint32_t Reader::pop_u32() noexcept
{
    if (!test_len(sizeof(uint32_t))) return 0;
    return pop_trivial<uint32_t>(m_data, m_offset);
}

int64_t Reader::pop_i64() noexcept
{
    if (!test_len(sizeof(int64_t))) return 0;
    return pop_trivial<int64_t>(m_data, m_offset);
}

uint64_t Reader::pop_u64() noexcept
{
    if (!test_len(sizeof(uint64_t))) return 0;
    return pop_trivial<uint64_t>(m_data, m_offset);
}

float Reader::pop_float() noexcept
{
    if (!test_len(sizeof(float))) return 0.0f;
    return pop_trivial<float>(m_data, m_offset);
}

double Reader::pop_double() noexcept
{
    if (!test_len(sizeof(double))) return 0.0f;
    return pop_trivial<double>(m_data, m_offset);
}

#ifndef TOOLS_DETACHED_STATE
number Reader::pop_number() noexcept
{
    static_assert(std::same_as<number_t, double>, "Update this function if number_t changes");
    if (!test_len(sizeof(double))) return 0.0f;
    return pop_trivial<double>(m_data, m_offset);
}
#endif // TOOLS_DETACHED_STATE

void Reader::pop_buff(float* arr) noexcept
{
    const size_t len = pop_u32();
    if (0 == len || !test_len(len)) return;
    std::memcpy(arr, m_data + m_offset, len);
    m_offset += len;
}

void Reader::pop_buff(byte* data) noexcept
{
    const size_t len = pop_u32();
    if (0 == len || !test_len(len)) return;
    std::memcpy(data, m_data + m_offset, len);
    m_offset += len;
}

byte_buff Reader::pop_buff()
{
    const size_t len = pop_u32();
    if (0 == len || !test_len(len)) return {};
    byte_buff buff;
    buff.resize(len);
    std::memcpy(buff.data(), m_data + m_offset, len);
    m_offset += len;
    return buff;
}

std::string Reader::pop_string()
{
    std::string str(pop_string_ptr());
    return str;
}

std::string_view Reader::pop_string_ptr() noexcept
{
    const uint32_t len = pop_u32();
    if (0 == len || !test_len(len))
    {
        static const char* empty = "";
        return std::string_view(empty);
    }
    std::string_view view(cast::ptrs<char>(m_data + m_offset), len);
    m_offset += len;
    return view;
}

std::wstring Reader::pop_wstring()
{
    const uint32_t len = pop_u32(); 
    if (0 == len || !test_len(len))
        return {};

    std::wstring str;
    str.resize(len / sizeof(wchar_t));

    std::memcpy(str.data(), m_data + m_offset, len);
    m_offset += len;
    return str;
}

#ifndef TOOLS_DETACHED_STATE
Primitive Reader::pop_primitive()
{
    Primitive ret;
    const byte type = pop_u8();
    switch (type)
    {
    case pt_i64:
        ret = pop_i64();
        break;
    case pt_double:
        ret = pop_double();
        break;
    case pt_number:
        ret = pop_number();
        break;
    case pt_bool:
        ret = pop_bool();
        break;
    case pt_string:
        ret = pop_string();
        break;
    case pt_monostate:
        // do nothing
        break;
    default:
        LogError("Undefined primitive type");
    }
    return ret;
}

ImVec2 Reader::pop_vec2() noexcept
{
    if (!test_len(sizeof(float) * 2)) return {};
    return pop_trivial<ImVec2>(m_data, m_offset);
}

ImVec4 Reader::pop_vec4() noexcept
{
    if (!test_len(sizeof(float) * 4)) return {};
    return pop_trivial<ImVec4>(m_data, m_offset);
}

ImRect Reader::pop_rect() noexcept
{
    if (!test_len(sizeof(float) * 4)) return {};
    return pop_trivial<ImRect>(m_data, m_offset);
}

ImU32 Reader::pop_clr() noexcept
{
    if (!test_len(sizeof(ImU32))) return 0;
    return pop_trivial<ImU32>(m_data, m_offset);
}

iid Reader::pop_iid() noexcept
{
    if (!test_len(sizeof(iid))) return 0;
    return pop_trivial<iid>(m_data, m_offset);
}

wid Reader::pop_wid() noexcept
{
    if (!test_len(sizeof(wid))) return 0;
    return pop_trivial<wid>(m_data, m_offset);
}

hid Reader::pop_hid() noexcept
{
    if (!test_len(sizeof(hid))) return 0;
    return pop_trivial<hid>(m_data, m_offset);
}
#endif // TOOLS_DETACHED_STATE

Blob Reader::pop_blob()
{
    byte_buff buff = pop_buff();
    Blob b(std::move(buff));
    return b;
}

Reader Reader::pop_block() noexcept
{
    const uint32_t len = pop_u32();
    if (!test_len(len) || 0 == len) return Reader({});
    Reader nested_reader(m_data + m_offset, len);
    m_offset += len;
    return nested_reader;
}

///////////////   Block
//////////////////////////////

Block::Block(Writer& writer, bool begin_block /*= true*/)
    : m_writer(writer)
{
    if (begin_block)
        begin();
}

Block::~Block() noexcept
{
    if (m_opened)
        end();
}

void Block::begin()
{
    LogRAssert(!m_opened, "Block already opened", );
    m_opened = true;

    m_writer.patch_begin();
    m_start_offset = m_writer.get()->size();
}

void Block::end() noexcept
{
    LogRAssert(m_opened, "Block wasn't open", );
    m_opened = false;

    const uint32_t payload_size = uint32_t(m_writer.get()->size() - m_start_offset);
    m_writer.patch_end(payload_size);
}

} // namespace blob
