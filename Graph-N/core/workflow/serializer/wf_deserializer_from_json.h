#pragma once

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;

class WfDeserializerFromJson final
{
public:
    WfDeserializerFromJson(Workflow* wf, const o_json& j);
    ~WfDeserializerFromJson() = default;

    NO_COPY_MOVE(WfDeserializerFromJson);

    bool deserialize();
    static bool deserialize_all_wf_names(const std::string& payload, std::list<std::pair<std::string, tools::dt>>& projects);

private:
    NodeInstance* deserialize_node(const o_json& j, std::string& node_id_from_json, bool dummy);
    bool deserialize_subgraph(const o_json& j);
    bool deserialize_links();
    NodeInstance* deserialize_group(const o_json& j);

    bool check_node_definition(const o_json& j, bool& is_valid) const;
    bool check_cni_node_definition(const o_json& j, hid node_class, bool& is_valid, bool& is_cni) const;
    bool check_cni_node_definition_primitive(const o_json& j, bool& is_valid) const;
    bool check_cni_node_definition_reroute(const o_json& j, bool& is_valid) const;
    bool check_cni_node_definition_note(const o_json& j, bool& is_valid) const;
    bool check_cni_node_definition_markdownnote(const o_json& j, bool& is_valid) const;
    bool check_cni_node_fast_switcher_rgthree(const o_json& j, bool& is_valid) const;
    bool check_cni_node_fast_group_switcher_rgthree(const o_json& j, bool& is_valid) const;
    bool check_cni_node_fast_action_rgthree(const o_json& j, bool& is_valid) const;
    bool check_cni_node_bookmark_rgthree(const o_json& j, bool& is_valid) const;
    bool check_cni_node_impact_switch(const o_json& j, bool& is_valid) const;
    bool check_cni_node_impact_insversed_switch(const o_json& j, bool& is_valid) const;

    NodeInstance* deserialize_js_node(NodeInstance* node, const o_json& j);

    NodeInstance* deserialize_cni_node(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_node_primitive(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_node_reroute(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_node_note(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_node_markdownnote(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_fast_switcher_rgthree(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_fast_group_switcher_rgthree(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_fast_action_rgthree(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_bookmark_rgthree(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_node_impact_switch(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_node_impact_insversed_switch(NodeInstance* node, const o_json& j);

private:
    Workflow* m_wf = nullptr;
    const o_json& m_json;

    using LinkMap = std::map<iid, std::map<int32_t, iid>>;  // node iid -> [pin index -> pin iid]
    using SubgraphMap = std::map<std::string, std::pair<std::string, ImRect>>; // subgraph uuid -> [subgraph id as node, subgraph pos]

    SubgraphMap m_subgraphs;
    LinkMap m_node_in_pins;
    LinkMap m_node_out_pins;
    std::map<std::string, iid> m_node_ids;
};

} // namespace data
