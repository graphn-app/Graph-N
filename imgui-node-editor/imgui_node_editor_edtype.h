#pragma once
#include <atomic>

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
    EdType() noexcept { ed_id = g_id.fetch_add(1, std::memory_order_relaxed);; }
    EdType(ax::NodeEditor::Detail::Node* n) noexcept : EdType() { obj = n; }
    EdType(ax::NodeEditor::Detail::Pin* p) noexcept : EdType() { obj = p; }
    EdType(ax::NodeEditor::Detail::Link* l) noexcept : EdType() { obj = l; }
    EdType(ax::NodeEditor::Detail::Object* o) noexcept : EdType() { obj = o; }

    EdType& operator=(const EdType& other) noexcept
    {
        if (this != &other)
        {
            obj = other.obj;
        }
        return *this;
    }

    void reset() noexcept
    {
        obj = nullptr;
    }

    operator ax::NodeEditor::Detail::Node*() const noexcept
    {
        return reinterpret_cast<ax::NodeEditor::Detail::Node*>(obj);
    }
    operator ax::NodeEditor::Detail::Pin*() const noexcept
    {
        return reinterpret_cast<ax::NodeEditor::Detail::Pin*>(obj);
    }
    operator ax::NodeEditor::Detail::Link*() const noexcept
    {
        return reinterpret_cast<ax::NodeEditor::Detail::Link*>(obj);
    }
    operator ax::NodeEditor::Detail::Object*() const noexcept
    {
        return reinterpret_cast<ax::NodeEditor::Detail::Object*>(obj);
    }
    uint64_t id() const noexcept
    {
        return ed_id;
    }

    explicit operator bool() const noexcept
    {
        return obj != nullptr;
    }

private:
    void* obj = nullptr;
    uint64_t ed_id = 0;
    
};

