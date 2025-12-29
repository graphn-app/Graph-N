
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
    Instance(Workflow* wf)
        : base::InstanceID(wf->get_id())
        , m_wf(wf)
    {
    }
    ~Instance() override
    {}

    iid get_instance_id() const { return get_id(); }	// use this to save/load
    aid get_graph_id() const { return m_graph_id; }		// use this on graph

    void set_base(BaseObj* obj)
    {
        g::pFlow->subscribe_confyui_validity_state(this);
        m_base = obj;
        m_obj_id = 0;	// not null only in validation period
        make_graph_id();
        Log("Wf id = " << get_workflow_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_base->get_id());
    }

    BaseObj* get_base() const
    {
        return m_base;
    }

    // DataInvalidationHandler
    void invalidate_comfyui_objects() override
    {
        m_obj_id = 0;
        if (nullptr == m_base)
        {
            LogAndThrowInternalError(ExcepCategory::inconsistent_ptr, "Can't prepare to invalidation - no object")
        }
        m_obj_id = m_base->get_id();
        m_base = nullptr;
        Log("Preinvalidate: Wf id = " << get_workflow_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_obj_id);
    }

    // DataInvalidationHandler
    void resolve_comfyui_objects() override
    {
        m_base = nullptr;
        if (0 == m_obj_id)
        {
            "LIVE POINTERS", ("LIVE POINTERS", "No saved data to invalidate object");
        }
        m_base = find_obj(m_obj_id);
        if (nullptr == m_base)
        {
            Log("Invalidate ptr: Wf id = " << get_workflow_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_obj_id);
            m_wf->remove(m_obj_id);
            m_obj_id = 0;
        }
        else
        {
            Log("Invalidated: Wf id = " << get_workflow_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_obj_id);
        }
    }

    virtual void draw() const = 0;

protected:
    virtual void make_graph_id() = 0;
    virtual BaseObj* find_obj(hid obj_id) = 0;

protected:
    Workflow* m_wf = nullptr;
    BaseObj* m_base = nullptr;		// base node/pin
    hid m_obj_id = 0;				// unique hash-id of base node/pin
    aid m_graph_id = 0;				// Unique ID across all graphs
};

} // namespace ui
