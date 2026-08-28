#pragma once
#include <atomic>

namespace ax { namespace NodeEditor { namespace Detail
{
struct Node;
struct Pin;
struct Link;
struct Object;
} } }

class EdType final
{
public:
    explicit EdType() noexcept
    {
        static std::atomic_uint64_t g_id = 1;
        m_id = g_id.fetch_add(1, std::memory_order_relaxed);
    }
    ~EdType() noexcept = default;

    EdType(const EdType& other) noexcept { this->operator=(other); }
    EdType& operator=(const EdType& other) noexcept
    {
        if (this != &other)
        {
            m_obj = other.m_obj;
            m_id = other.m_id;
        }
        return *this;
    }

    //void init(ax::NodeEditor::Detail::Object* o) noexcept { m_obj = o; }
    void init(ax::NodeEditor::Detail::Node* o) noexcept { m_obj = reinterpret_cast<ax::NodeEditor::Detail::Object*>(o); }
    void init(ax::NodeEditor::Detail::Pin* o) noexcept { m_obj = reinterpret_cast<ax::NodeEditor::Detail::Object*>(o); }
    void init(ax::NodeEditor::Detail::Link* o) noexcept { m_obj = reinterpret_cast<ax::NodeEditor::Detail::Object*>(o); }

    bool has_object() const noexcept { return m_obj != nullptr; }
    void reset() noexcept { m_obj = nullptr; }

    ax::NodeEditor::Detail::Object* get_object() const noexcept { return static_cast<ax::NodeEditor::Detail::Object*>(m_obj); }
    ax::NodeEditor::Detail::Node* get_node() const noexcept { return static_cast<ax::NodeEditor::Detail::Node*>(m_obj); }
    ax::NodeEditor::Detail::Pin* get_pin() const noexcept { return static_cast<ax::NodeEditor::Detail::Pin*>(m_obj); }
    ax::NodeEditor::Detail::Link* get_link() const noexcept { return static_cast<ax::NodeEditor::Detail::Link*>(m_obj); }

    //operator ax::NodeEditor::Detail::Object*() const noexcept { return static_cast<ax::NodeEditor::Detail::Object*>(m_obj); }
    //operator ax::NodeEditor::Detail::Node*() const noexcept { return static_cast<ax::NodeEditor::Detail::Node*>(m_obj); }
    //operator ax::NodeEditor::Detail::Pin*() const noexcept { return static_cast<ax::NodeEditor::Detail::Pin*>(m_obj); }
    //operator ax::NodeEditor::Detail::Link*() const noexcept { return static_cast<ax::NodeEditor::Detail::Link*>(m_obj); }

    bool has_id() const noexcept { return m_id != 0; }
    uint64_t id() const noexcept { return m_id; }

private:
    void* m_obj = nullptr;
    uint64_t m_id = 0;
};

