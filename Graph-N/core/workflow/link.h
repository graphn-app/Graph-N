
#pragma once

namespace data
{
class PinInstance;

class Link final
{
public:
    Link(wid workflow_id, iid link_id);
    ~Link();

    static bool can_link(const PinInstance* p1, const PinInstance* p2);
    static Link* link(PinInstance* p1, PinInstance* p2);

    inline const PinInstance* get_input() const { return m_input; }
    inline const PinInstance* get_output() const { return m_output; }

    inline iid get_instance_id() const { return m_iid; }
    inline wid get_workflow_id() const { return m_workflow_id; }

    void unlink();

private:
    wid m_workflow_id = 0;
    iid m_iid = 0;
    const PinInstance* m_input = nullptr;
    const PinInstance* m_output = nullptr;
};

} // namespace data
