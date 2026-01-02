
#pragma once
#include "core/managers/comfyui_object_validity_listener.h"
#include "core/workflow/workflow.h"
#include "ui/graph-n_flow.h"

namespace data
{

template <typename BaseObj>                    // base data::Pin or data::Node
class Instance :
    public base::LocalID,
    public mngr::ComfyUiObjectValidityListener
{
public:
    Instance(Workflow* wf, BaseObj* obj, iid instance_id, gid graph_id)
        : m_wf(wf)
        , m_object(obj)
        , m_graph_id(graph_id)
        , m_instance_id(instance_id)
    {
        m_graph_id_str = std::to_string(m_graph_id);
    }
    ~Instance() override
    {}

    virtual void init()
    {
        g::pFlow->subscribe_comfyui_validity_state(this);
        Log("Wf id = " << m_wf->get_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_object->get_id());
    }

    iid get_instance_id() const { return m_instance_id; }    // use this to save/load
    gid get_graph_id() const { return m_graph_id; }     // use this on graph
    const std::string& get_graph_id_str() const { return m_graph_id_str; }
    hid get_object_id() const { return m_obj_id; }      // raw node/pin id

    Workflow* get_workflow() const
    {
        return m_wf;
    }

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
        Log("Preinvalidate: Wf id = " << m_wf->get_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_obj_id);
    }

    // ComfyUiObjectValidityListener
    void begin_resolve_comfyui_objects() override
    {
        m_object = nullptr;
        if (0 == m_obj_id)
        {
            LogError(ExcepCategory::inconsistent_ptr, "No saved data to invalidate object");
        }
        m_object = find_obj(m_obj_id);
        if (nullptr == m_object)
        {
            Log("Invalid obj: Wf id = " << m_wf->get_id() << ", Inst id=" << get_instance_id() << ", Graph id=" << get_graph_id() << ", Obj id=" << m_obj_id);
            m_wf->mark_invalid(m_obj_id);
            m_obj_id = 0;
        }
    }

protected:
    virtual BaseObj* find_obj(hid obj_id) = 0;

protected:
    Workflow* m_wf = nullptr;
    BaseObj* m_object = nullptr;     // base node/pin
    hid m_obj_id = 0;                // unique hash-id of base node/pin. Not null only in validation period
    gid m_graph_id = 0;              // Unique ID across all graphs
    std::string m_graph_id_str;
    iid m_instance_id = 0;
};

} // namespace data
