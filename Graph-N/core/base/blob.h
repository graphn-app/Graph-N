/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

///////////////////////////////////////////////////////////////////
///////      blob                 /////////////////////////////////
///////////////////////////////////////////////////////////////////

namespace blob
{

class Blob final
{
public:
    Blob() noexcept = default;
    Blob(const void* buffer, size_t len)
    {
        m_blob.resize(len);
        memcpy(m_blob.data(), buffer, len);
    }
    ~Blob() noexcept = default;

    Blob(const Blob&) = delete;
    Blob& operator=(const Blob&) = delete;

    Blob(Blob&& other) noexcept
        : m_blob(std::move(other.m_blob))
    { }
    Blob& operator=(Blob&& other) noexcept
    {
        if (this != &other) m_blob = std::move(other.m_blob);
        return *this;
    }

    Blob(byte_buff&& buffer) noexcept
        : m_blob(std::move(buffer))
    { }
    Blob& operator=(byte_buff&& buffer) noexcept
    {
        m_blob = std::move(buffer);
        return *this;
    }

    bool operator==(const Blob& other) const noexcept
    {
        if (m_blob.size() != other.m_blob.size())
            return false;
        return memcmp(m_blob.data(), other.m_blob.data(), m_blob.size()) == 0;
    }
    bool operator!=(const Blob& other) const noexcept
    {
        return !(*this == other);
    }

    byte_buff* operator->() noexcept { return &m_blob; }
    const byte_buff* operator->() const noexcept { return &m_blob; }

    byte_buff& operator*() noexcept { return m_blob; }
    const byte_buff& operator*() const noexcept { return m_blob; }

    Blob copy() const
    {
        Blob out;
        out.m_blob.resize(m_blob.size());
        memcpy(out.m_blob.data(), m_blob.data(), m_blob.size());
        return out;
    }

private:
    byte_buff m_blob;
};
} // namespace blob

