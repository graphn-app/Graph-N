
#pragma once

namespace data
{
class Workflow;
class PinInstance;

class Link final
{
public:
    Link(Workflow* wf, gid graph_id);
    ~Link();

    static bool can_link(const PinInstance* p1, const PinInstance* p2);
    static Link* link(PinInstance* p1, PinInstance* p2);

    PinInstance* get_input() const { return m_input; }
    PinInstance* get_output() const { return m_output; }

    gid get_graph_id() const { return m_graph_id; }     // use this on graph

    void unlink();

private:
    Workflow* m_wf = nullptr;
    PinInstance* m_input = nullptr;
    PinInstance* m_output = nullptr;

    gid m_graph_id = 0;
    hid input_id = 0;
    hid output_id = 0;
};

} // namespace data
