#pragma once

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;

class WfDeserializerFromBlob final
{
public:
    WfDeserializerFromBlob(Workflow* wf) noexcept;
    WfDeserializerFromBlob(Workflow* wf, const blob::Blob& blob) noexcept;
    ~WfDeserializerFromBlob() noexcept = default;

    NO_COPY_MOVE(WfDeserializerFromBlob);

    bool deserialize(blob::Blob& drawer_state);
    NodeInstance* deserialize_node(blob::Reader& block);

private:
    NodeInstance* deserialize_node(blob::Reader& block, bool dummy);    
    bool deserialize_pin(blob::Reader& block, bool dummy);
    void deserialize_link(blob::Reader& block);

    bool check_node_definition(blob::Reader& block) const;
    bool check_cni_node_definition(blob::Reader& block, const std::string& node_class) const;

    NodeInstance* deserialize_js_node(blob::Reader& reader);

    NodeInstance* deserialize_cni_node(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_primitive(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_reroute(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_note(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_markdownnote(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_fast_switcher_rgthree(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_fast_group_switcher_rgthree(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_fast_action_rgthree(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_bookmark_rgthree(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_impact_switch(blob::Reader& reader);
    NodeInstance* deserialize_cni_node_impact_insversed_switch(blob::Reader& reader);

private:
    Workflow* m_wf = nullptr;
    blob::Reader m_reader;
    NodeInstance* m_curr_node = nullptr;
    PinInstance* m_curr_pin = nullptr;
};

} // namespace data
