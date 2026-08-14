#pragma once
#include <atomic>
#include <assert.h>

namespace ax { namespace NodeEditor { namespace Detail
{
struct Node;
struct Pin;
struct Link;
struct Object;
} } }

namespace {
static std::atomic_uint64_t g_id = 1;
}

class EdType
{
public:
    EdType(bool assign_id = true) noexcept
    {
        if (assign_id)
        {
            m_id = g_id.fetch_add(1, std::memory_order_relaxed);
        }
    }

    EdType(ax::NodeEditor::Detail::Node* n) noexcept
        : EdType()
    {
        assert(m_id != 0);
        m_obj = n;
    }

    EdType(ax::NodeEditor::Detail::Pin* p) noexcept
        : EdType()
    {
        assert(m_id != 0);
        m_obj = p;
    }
    EdType(ax::NodeEditor::Detail::Link* l) noexcept
        : EdType()
    {
        assert(m_id != 0);
        m_obj = l;
    }
    EdType(ax::NodeEditor::Detail::Object* o) noexcept
        : EdType()
    {
        assert(m_id != 0);
        m_obj = o;
    }

    EdType& operator=(const EdType& other) noexcept
    {
        if (this != &other)
        {
            m_obj = other.m_obj;
        }
        return *this;
    }

    void reset() noexcept
    {
        m_obj = nullptr;
    }

    operator ax::NodeEditor::Detail::Node*() const noexcept
    {
        return static_cast<ax::NodeEditor::Detail::Node*>(m_obj);
    }
    operator ax::NodeEditor::Detail::Pin*() const noexcept
    {
        return static_cast<ax::NodeEditor::Detail::Pin*>(m_obj);
    }
    operator ax::NodeEditor::Detail::Link*() const noexcept
    {
        return static_cast<ax::NodeEditor::Detail::Link*>(m_obj);
    }
    operator ax::NodeEditor::Detail::Object*() const noexcept
    {
        return static_cast<ax::NodeEditor::Detail::Object*>(m_obj);
    }
    uint64_t id() const noexcept
    {
        return m_id;
    }

    explicit operator bool() const noexcept
    {
        return m_obj != nullptr;
    }
    bool empty() const noexcept
    {
        return m_id != 0;
    }

private:
    void* m_obj = nullptr;
    uint64_t m_id = 0;
    
};

