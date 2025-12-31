
#pragma once
#include "core/managers/comfyui_object_validity_listener.h"
#include "ui/graph-n_flow.h"
#include "ui/workflow/workflow.h"

namespace ui
{

template <typename BaseObj>					// base data::Pin or data::Node
class Instance :
    public base::InstanceID,
    public mngr::ComfyUiObjectValidityListener
{
public:
    Instance(Workflow* wf, BaseObj* obj)
        : base::InstanceID(wf->get_id())
        , m_wf(wf)
        , m_object(obj)
    {
    }
    ~Instance() override
    {}

    virtual void init()
    {
        g::pFlow->subscribe_comfyui_validity_state(this);
        make_graph_id();
        Log("Wf id = " << get_workflow_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_object->get_id());
    }

    virtual void draw() = 0;

    iid get_instance_id() const { return get_id(); }	// use this to save/load
    aid get_graph_id() const { return m_graph_id; }		// use this on graph
    hid get_object_id() const { return m_obj_id; }      // raw node/pin id

    BaseObj* get_object() const
    {
        return m_object;
    }

    bool is_valid() const
    {
        return m_obj_id != 0 || m_object != nullptr;
    }

    // ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() override
    {
        m_obj_id = 0;
        if (nullptr == m_object)
        {
            LogAndThrowInternalError(ExcepCategory::inconsistent_ptr, "Can't prepare to invalidation - no object")
        }
        m_obj_id = m_object->get_id();
        m_object = nullptr;
        Log("Preinvalidate: Wf id = " << get_workflow_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_obj_id);
    }

    // ComfyUiObjectValidityListener
    void resolve_comfyui_objects() override
    {
        m_object = nullptr;
        if (0 == m_obj_id)
        {
            LogError(ExcepCategory::inconsistent_ptr, "No saved data to invalidate object");
        }
        m_object = find_obj(m_obj_id);
        if (nullptr == m_object)
        {
            Log("Invalid obj: Wf id = " << get_workflow_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_obj_id);
            m_wf->remove_invalid(m_obj_id);
            m_obj_id = 0;
        }
    }

protected:
    virtual void make_graph_id() = 0;
    virtual BaseObj* find_obj(hid obj_id) = 0;

protected:
    Workflow* m_wf = nullptr;
    BaseObj* m_object = nullptr;		// base node/pin
    hid m_obj_id = 0;				// unique hash-id of base node/pin. Not null only in validation period
    aid m_graph_id = 0;				// Unique ID across all graphs
};

} // namespace ui
