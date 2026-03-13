
#include "tools/blob_serializer.h"

namespace blob
{
///////////////   primitive
//////////////////////////////

enum PrimitiveType
{
    pt_i64,
    pt_u64,
    pt_double,
    pt_bool,
    pt_string,
    pt_monostate
};


template <typename TRIVIAL>
void push_trivial(TRIVIAL val, Blob& buffer)
{
    static_assert(std::is_trivially_copyable_v<TRIVIAL>, "Type is not safe for memcpy!");
    const byte* ptr = reinterpret_cast<const byte*>(&val);
    buffer->insert(buffer->end(), ptr, ptr + sizeof(TRIVIAL));
}

template <typename TRIVIAL>
TRIVIAL pop_trivial(const byte* data, size_t& offset)
{
    static_assert(std::is_trivially_copyable_v<TRIVIAL>, "Type is not safe for memcpy!");
    TRIVIAL value = {};
    std::memcpy(&value, data + offset, sizeof(TRIVIAL));
    offset += sizeof(TRIVIAL);
    return value;
}

///////////////   Writer
//////////////////////////////   

Writer::Writer() = default;
Writer::~Writer() = default;

Writer::Writer(Writer&& other)
{
    *this = std::move(other);
}

Writer& Writer::operator=(Writer&& other)
{
    if (this == &other)
        return *this;

    m_buffer = std::move(other.m_buffer);
    return *this;
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

Writer& Writer::push(const byte* data, size_t len)
{
    push(cast::to_u32(len));
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

Writer& Writer::push(const primitive& val)
{
    std::visit([this](const auto& v)
    {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, int64_t>)
        {
            push(uint8_t(pt_i64)).push(v);
        }
        else if constexpr (std::is_same_v<T, uint64_t>)
        {
            push(uint8_t(pt_u64)).push(v);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            push(uint8_t(pt_double)).push(v);
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
    }, val);

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

Writer& Writer::push(const Blob& val)
{
    push(*val);
    return *this;
}

Writer& Writer::push(const Writer& val)
{
    push(cast::to_u32((val.m_buffer->size())));
    if (!val.m_buffer->empty())
        push(val.m_buffer->data(), val.m_buffer->size());    
    return *this;
}

void Writer::patch(size_t offset, uint32_t val)
{
    if (offset + sizeof(uint32_t) > m_buffer->size())
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Patch: Out of range");
    }

    std::memcpy(m_buffer->data() + offset, &val, sizeof(uint32_t));
}

///////////////   Reader
//////////////////////////////   

Reader::Reader(const Blob& buffer)
    : m_data(buffer->data())
    , m_size(buffer->size())
    , m_offset(0)
{}
Reader::Reader(const byte* data, size_t size)
    : m_data(data)
    , m_size(size)
    , m_offset(0)
{}
Reader::~Reader() = default;

Reader::Reader(Reader&& other)
    : m_data(std::exchange(other.m_data, nullptr))
    , m_size(std::exchange(other.m_size, 0))
    , m_offset(std::exchange(other.m_offset, 0))
{}

Reader& Reader::operator=(Reader&& other)
{
    if (this == &other)
        return *this;

    m_data = std::exchange(other.m_data, nullptr);
    m_size = std::exchange(other.m_size, 0);
    m_offset = std::exchange(other.m_offset, 0);
    return *this;
}

bool Reader::empty() const
{
    return m_offset >= m_size;
}

void Reader::test_len(size_t len) const
{
    if (len > m_size - m_offset)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Reader len: Out of range");
    }
}

bool Reader::pop_bool()
{
    test_len(sizeof(bool));
    return pop_trivial<bool>(m_data, m_offset);
}

uint8_t Reader::pop_u8()
{
    test_len(sizeof(uint8_t));
    return pop_trivial<uint8_t>(m_data, m_offset);
}


int32_t Reader::pop_i32()
{
    test_len(sizeof(int32_t));
    return pop_trivial<int32_t>(m_data, m_offset);
}

uint32_t Reader::pop_u32()
{
    test_len(sizeof(uint32_t));
    return pop_trivial<uint32_t>(m_data, m_offset);
}

int64_t Reader::pop_i64()
{
    test_len(sizeof(int64_t));
    return pop_trivial<int64_t>(m_data, m_offset);
}

uint64_t Reader::pop_u64()
{
    test_len(sizeof(uint64_t));
    return pop_trivial<uint64_t>(m_data, m_offset);
}

float Reader::pop_float()
{
    test_len(sizeof(float));
    return pop_trivial<float>(m_data, m_offset);
}

double Reader::pop_double()
{
    test_len(sizeof(double));
    return pop_trivial<double>(m_data, m_offset);
}

void Reader::pop_buff(float* arr)
{
    const size_t len = pop_u32();
    test_len(len);
    std::memcpy(arr, m_data + m_offset, len);
    m_offset += len;
}

void Reader::pop_buff(byte* data)
{
    const size_t len = pop_u32();
    test_len(len);
    std::memcpy(data, m_data + m_offset, len);
    m_offset += len;
}

byte_buff Reader::pop_buff()
{
    const size_t len = pop_u32();
    test_len(len);
    byte_buff buff;
    buff.resize(len);
    std::memcpy(buff.data(), m_data + m_offset, len);
    m_offset += len;
    return buff;
}

//
//std::string Reader::pop_string()
//{
//    const uint64_t len = pop_u64();
//    test_len(len);
//    std::string str(reinterpret_cast<const char*>(m_data + m_offset), len);
//    m_offset += len;
//    return str;
//}

std::string_view Reader::pop_string()
{
    const uint32_t len = pop_u32();
    test_len(len);
    std::string_view view(cast::ptrs<char>(m_data + m_offset), len);
    m_offset += len;
    return view;
}

primitive Reader::pop_primitive()
{
    primitive ret;
    const byte type = pop_u8();
    switch (type)
    {
    case pt_i64:
        ret = pop_i64();
        break;
    case pt_u64:
        ret = pop_u64();
        break;
    case pt_double:
        ret = pop_double();
        break;
    case pt_bool:
        ret = pop_bool();
        break;
    case pt_string:
        ret = std::string(pop_string());
        break;
    case pt_monostate:
        ret = std::monostate();
        break;
    default:
        {
            LogAndThrowInternalError(ExcepCategory::Logic, "Undefined primitive type");
        }
    }
    return ret;
}

ImVec2 Reader::pop_vec2()
{
    test_len(sizeof(float) * 2);
    return pop_trivial<ImVec2>(m_data, m_offset);
}

ImVec4 Reader::pop_vec4()
{
    test_len(sizeof(float) * 4);
    return pop_trivial<ImVec4>(m_data, m_offset);
}

ImRect Reader::pop_rect()
{
    test_len(sizeof(float) * 4);
    return pop_trivial<ImRect>(m_data, m_offset);
}

ImU32 Reader::pop_clr()
{
    test_len(sizeof(ImU32));
    return pop_trivial<ImU32>(m_data, m_offset);
}

iid Reader::pop_iid()
{
    test_len(sizeof(iid));
    return pop_trivial<iid>(m_data, m_offset);
}

wid Reader::pop_wid()
{
    test_len(sizeof(wid));
    return pop_trivial<wid>(m_data, m_offset);
}

hid Reader::pop_hid()
{
    test_len(sizeof(hid));
    return pop_trivial<hid>(m_data, m_offset);
}

Blob Reader::pop_blob()
{
    byte_buff buff = pop_buff();
    Blob b(std::move(buff));
    return b;
}

Reader Reader::pop_block()
{
    const uint32_t len = pop_u32();
    test_len(len);
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

Block::~Block()
{
    if (m_opened)
        end();
}

void Block::begin()
{
    if (m_opened) { LogAndThrowInternalError(ExcepCategory::Logic, "Block already open"); }
    m_opened = true;

    m_size_offset = m_writer.get()->size();
    m_writer.push(uint32_t(0));
    m_start_offset = m_writer.get()->size();
}

void Block::end()
{
    if (!m_opened) { LogAndThrowInternalError(ExcepCategory::Logic, "Block wasn't open"); }
    m_opened = false;

    const uint32_t payload_size = uint32_t(m_writer.get()->size() - m_start_offset);
    m_writer.patch(m_size_offset, payload_size);
}

} // namespace blob
