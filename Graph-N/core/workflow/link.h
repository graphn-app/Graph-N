
#pragma once
#include "imgui_node_editor_edtype.h"

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

    iid get_instance_id() const noexcept { return m_iid; }
    wid get_workflow_id() const noexcept { return m_workflow_id; }

    bool is_linked_to(const PinInstance* pin) const noexcept { return pin == m_input || pin == m_output; }

    void reset_state() noexcept;

    void unlink();

    EdType& ed() noexcept { return m_ed; }
    const EdType& ed() const noexcept { return m_ed; }

private:
    wid m_workflow_id = 0;
    iid m_iid = 0;
    PinInstance* m_input = nullptr;
    PinInstance* m_output = nullptr;

    EdType m_ed;
};

} // namespace data
