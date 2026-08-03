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

    NodeInstance* deserialize_cni_node(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_node_primitive(NodeInstance* node, const o_json& j);
    NodeInstance* deserialize_cni_node_reroute(NodeInstance* node, const o_json& j);

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
