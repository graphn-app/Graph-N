
#pragma once
#include "core/workflow/workflow.h"
#include "core/data/node.h"
#include "imgui_node_editor_edtype.h"

namespace data
{

template <typename BaseObj>
class Instance
{
public:
    Instance(BaseObj* obj, wid workflow_id, iid instance_id)
        : m_object(obj)
        , m_wid(workflow_id)
        , m_iid(instance_id)
        , m_obj_hid(nullptr == obj ? 0 : obj->get_id())
    {
    }
    virtual ~Instance()
    {
    }

    virtual void init()
    {
        if (m_object != nullptr && 0 == m_obj_hid)
            m_obj_hid = m_object->get_id();
        Log("Obj init wid=" << m_wid << ", iid=" << m_iid << ", hid=" << m_obj_hid);
    }

    inline iid get_instance_id() const { return m_iid; }
    inline wid get_workflow_id() const { return m_wid; }
    inline hid get_obj_id() const { return m_obj_hid; }

    inline BaseObj* get_object() const { return m_object; }
    virtual void set_object(BaseObj* obj) { m_object = obj; m_obj_hid = obj->get_id(); }

    inline EdType& ed() { return m_ed; }
    inline const EdType& ed() const { return m_ed; }
    virtual void reset_state() { m_ed.reset(); }

protected:
    BaseObj* m_object = nullptr;     // base node/pin
    std::string m_obj_class;
    wid m_wid = 0;
    iid m_iid = 0;
    hid m_obj_hid = 0;

private:
    EdType m_ed;
};

} // namespace data
