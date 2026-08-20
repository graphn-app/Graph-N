
#pragma once
#include "core/workflow/workflow.h"
#include "core/workflow/node.h"
#include "core/managers/workflows_manager.h"
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

#ifdef _DEBUG
    void set_instance_id(iid id) noexcept { m_iid = id; auto wf = g::pWfsManager->access(m_wid); wf->test_duplicates(); }
#else
    void set_instance_id(iid id) noexcept { m_iid = id; }
#endif
    void set_workflow_id(wid id) noexcept { m_wid = id; }

    virtual BaseObj* get_object() const noexcept { return m_object; }
    virtual void set_object(BaseObj* obj) { m_object = obj; }

    void hide_instance(bool hide) noexcept { m_hidden = hide; }
    bool is_hidden() const noexcept { return m_hidden; }

    EdType& ed() noexcept { return m_ed; }
    const EdType& ed() const noexcept { return m_ed; }
    virtual uint64_t reset_state() noexcept { const uint64_t id = m_ed.id(); m_ed.reset(); return id; }

protected:
    wid m_wid = 0;
    iid m_iid = 0;

private:
    BaseObj* m_object = nullptr;     // base node/pin
    EdType m_ed;
    bool m_hidden = false;
};

} // namespace data
