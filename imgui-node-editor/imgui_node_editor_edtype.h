
#pragma once

namespace ax
{
namespace NodeEditor
{
namespace Detail
{
struct Node;
struct Pin;
struct Link;
struct Object;
}
}
}

class EdType
{
public:
    EdType() {}
    EdType(ax::NodeEditor::Detail::Node* n) : obj(n) {}
    EdType(ax::NodeEditor::Detail::Pin* p) : obj(p) {}
    EdType(ax::NodeEditor::Detail::Link* l) : obj(l) {}
    EdType(ax::NodeEditor::Detail::Object* o) : obj(o) {}

    inline EdType& operator=(const EdType& other)
    {
        if (this != &other)
        {
            obj = other.obj;
        }
        return *this;
    }

    inline void reset()
    {
        obj = nullptr;
    }

    inline operator ax::NodeEditor::Detail::Node*() const
    {
        return reinterpret_cast<ax::NodeEditor::Detail::Node*>(obj);
    }
    inline operator ax::NodeEditor::Detail::Pin*() const
    {
        return reinterpret_cast<ax::NodeEditor::Detail::Pin*>(obj);
    }
    inline operator ax::NodeEditor::Detail::Link*() const
    {
        return reinterpret_cast<ax::NodeEditor::Detail::Link*>(obj);
    }
    inline operator ax::NodeEditor::Detail::Object*() const
    {
        return reinterpret_cast<ax::NodeEditor::Detail::Object*>(obj);
    }

    inline explicit operator bool() const
    {
        return obj != nullptr;
    }

private:
    void* obj = nullptr;
};
