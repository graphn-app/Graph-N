
#pragma once
#include "core/listeners.h"
#include "core/workflow/workflow.h"
#include "core/data/node.h"
#include "ui/graph-n_flow.h"
#include "imgui_node_editor_edtype.h"

namespace data
{

template <typename BaseObj>                    // base data::Pin or data::Node
class Instance :
    public listener::ComfyUiObjectValidityListener
{
public:
    Instance(BaseObj* obj, wid workflow_id, iid instance_id)
        : m_object(obj)
        , m_wid(workflow_id)
        , m_iid(instance_id)
        , m_hid(nullptr == obj ? 0 : obj->get_id())
    {
    }
    ~Instance() override
    {}

    virtual void init()
    {
        g::pFlow->subscribe_comfyui_validity_state(this);
        Log("Obj init wid=" << m_wid << ", iid=" << m_iid << ", hid=" << m_hid);
    }

    inline iid get_instance_id() const { return m_iid; }
    inline wid get_workflow_id() const { return m_wid; }
    inline BaseObj* get_object() const { return m_object; }

    inline bool is_valid() const { return m_hid != 0 || m_object != nullptr; }

    // ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() override
    {
        m_hid = 0;
        if (nullptr == m_object)
        {
            LogInternalPtr("Can't prepare to invalidation - no object");
        }
        m_hid = m_object->get_id();
        m_object = nullptr;
        Log("Preinvalidate: wid=" << m_wid << ", iid=" << m_iid << ", hid=" << m_hid);
    }

    // ComfyUiObjectValidityListener
    void begin_resolve_comfyui_objects() override
    {
        m_object = nullptr;
        if (0 == m_hid)
        {
            LogInternalPtr("No saved data to invalidate object");
        }
        m_object = find_obj(m_hid);
        if (nullptr == m_object)
        {
            Log("Invalid obj: wid=" << m_wid << ", iid=" << m_iid << ", hid=" << m_hid);
            //m_wf->mark_invalid(m_hid); //  TODO
            m_hid = 0;
        }
    }

    inline EdType& ed() { return m_ed; }
    inline const EdType& ed() const { return m_ed; }

protected:
    virtual BaseObj* find_obj(hid obj_id) const = 0;

protected:
    BaseObj* m_object = nullptr;     // base node/pin
    wid m_wid = 0;
    iid m_iid = 0;
    hid m_hid = 0;

private:
    EdType m_ed;
};

} // namespace data
