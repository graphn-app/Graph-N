#pragma once
#include "imgui_node_editor_edtype.h"
#include "core/managers/workflows_manager.h"

namespace data
{
class PinInstance;
class Workflow;

class Link final
{
public:
    Link(wid workflow_id, iid link_id) noexcept;
    ~Link() noexcept;

    static bool can_link(const PinInstance* p1, const PinInstance* p2);
    static Link* link(PinInstance* p1, PinInstance* p2, iid instance_id = 0);

    PinInstance* get_input() const noexcept { return m_input; }
    PinInstance* get_output() const noexcept { return m_output; }

    float get_link_thickness() const noexcept { return m_link_thickness; }
    void set_link_thickness(float thickness) noexcept { m_link_thickness = thickness; }

    iid get_instance_id() const noexcept { return m_iid; }
    void set_instance_id(iid id)
    {
        m_iid = id;
        auto wf = g::pWfsManager->access(m_workflow_id);
        wf->set_last_child_id(id);
        wf->test_duplicates();
    }

    wid get_workflow_id() const noexcept { return m_workflow_id; }

    bool is_linked_to(const PinInstance* pin) const noexcept { return pin == m_input || pin == m_output; }

    bool is_reroute_splitting_link() const noexcept;

    uint64_t reset_state() noexcept;

    void unlink();

    EdType& ed() noexcept { return m_ed; }
    const EdType& ed() const noexcept { return m_ed; }

private:
    wid m_workflow_id = 0;
    iid m_iid = 0;
    PinInstance* m_input = nullptr;
    PinInstance* m_output = nullptr;
    float m_link_thickness = 0.0f;
    EdType m_ed;
};

} // namespace data
