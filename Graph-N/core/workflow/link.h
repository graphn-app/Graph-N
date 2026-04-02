
#pragma once
#include "imgui_node_editor_edtype.h"

namespace data
{
class PinInstance;
class Workflow;

class Link final
{
    friend class WorkflowSerializerToBlob;
    friend class WorkflowSerializerToJson;

public:
    Link(wid workflow_id, iid link_id);
    ~Link();

    static bool can_link(const PinInstance* p1, const PinInstance* p2);
    static Link* link(PinInstance* p1, PinInstance* p2, iid instance_id = 0);

    inline const PinInstance* get_input() const { return m_input; }
    inline const PinInstance* get_output() const { return m_output; }

    inline iid get_instance_id() const { return m_iid; }
    inline wid get_workflow_id() const { return m_workflow_id; }

    inline bool is_linked_to(const PinInstance* pin) const { return pin == m_input || pin == m_output; }

    void reset_state();

    void unlink();

    inline EdType& ed() { return m_ed; }
    inline const EdType& ed() const { return m_ed; }

private:
    wid m_workflow_id = 0;
    iid m_iid = 0;
    PinInstance* m_input = nullptr;
    PinInstance* m_output = nullptr;

    EdType m_ed;
};

} // namespace data
