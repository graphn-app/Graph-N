
#pragma once
#include "core/workflow/workflow.h"
#include "core/workflow/node.h"
#include "imgui_node_editor_edtype.h"

namespace data
{

template <typename BaseObj>
class Instance
{
public:
    Instance(BaseObj* obj, wid workflow_id, iid instance_id) noexcept
        : m_object(obj)
        , m_wid(workflow_id)
        , m_iid(instance_id)
    {}
    virtual ~Instance() noexcept = default;

    virtual void init() {}

    iid get_instance_id() const noexcept { return m_iid; }
    wid get_workflow_id() const noexcept { return m_wid; }

    void set_instance_id(iid id) noexcept { m_iid = id; }
    void set_workflow_id(wid id) noexcept { m_wid = id; }

    virtual BaseObj* get_object() const noexcept { return m_object; }
    virtual void set_object(BaseObj* obj) { m_object = obj; }

    virtual EdType& ed() noexcept { return m_ed; }
    virtual const EdType& ed() const noexcept { return m_ed; }
    virtual void reset_state() noexcept { m_ed.reset(); }

protected:
    wid m_wid = 0;
    iid m_iid = 0;

private:
    BaseObj* m_object = nullptr;     // base node/pin
    EdType m_ed;
};

} // namespace data
