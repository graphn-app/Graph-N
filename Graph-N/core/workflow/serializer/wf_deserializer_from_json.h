/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace data
{
class Workflow;
struct Node;
class NodeInstance;
class PinInstance;
class Link;

struct ParseResult final
{
    Node* node_obj = nullptr;
    NodeInstance* node_inst = nullptr;
    ImRect rc = {};
    std::string node_class;
    bool is_cni_node = false;
    bool subgraph = false;

    bool invalid_definition = false;                // required item is missing (type, id etc)
    bool unrecognized_node_class = false;           // unknown node
    bool unrecognized_node_definition = false;      // node definition doesn't match
};

class WfDeserializerFromJson final
{
public:
    WfDeserializerFromJson(Workflow* wf, const o_json& j);
    ~WfDeserializerFromJson() noexcept = default;

    NO_COPY_MOVE(WfDeserializerFromJson);

    bool deserialize();
    static bool deserialize_all_wf_names(const std::string& payload, std::list<std::pair<std::string, tools::dt>>& projects);

private:
    ParseResult deserialize_node(const o_json& j, std::string& node_id_from_json);
    NodeInstance* deserialize_group(const o_json& j);
    bool deserialize_subgraph(const o_json& j);
    bool deserialize_links();
    bool handle_node_errors(const o_json& jnode, ParseResult& result);

    void deserialize_js_node(const o_json& j, ParseResult& result) noexcept;
    void deserialize_cni_node(const o_json& j, ParseResult& result);
    void deserialize_cni_node_primitive(const o_json& j, ParseResult& result);
    void deserialize_cni_node_reroute(const o_json& j, ParseResult& result);
    void deserialize_cni_node_note(const o_json& j, ParseResult& result);
    void deserialize_cni_node_markdownnote(const o_json& j, ParseResult& result);
    void deserialize_cni_fast_switcher_rgthree(const o_json& j, ParseResult& result);
    void deserialize_cni_fast_group_switcher_rgthree(const o_json& j, ParseResult& result);
    void deserialize_cni_fast_action_rgthree(const o_json& j, ParseResult& result);
    void deserialize_cni_node_power_lora_loader_rgthree(const o_json& j, ParseResult& result);
    void deserialize_cni_node_impact_switch(const o_json& j, ParseResult& result);
    void deserialize_cni_node_impact_insversed_switch(const o_json& j, ParseResult& result);

private:
    Workflow* m_wf = nullptr;
    const o_json& m_json;

    using LinkMap = std::map<iid, std::map<int32_t, iid>>;  // node iid -> [pin index -> pin iid]
    using SubgraphMap = std::map<std::string, std::pair<std::string, ImRect>>; // subgraph uuid -> [subgraph id as node, subgraph pos]

    SubgraphMap m_subgraphs;
    LinkMap m_node_in_pins;
    LinkMap m_node_out_pins;
    std::map<std::string, iid> m_node_ids;

    static std::unordered_set<std::string> m_ignore_pin_types;
    std::unordered_set<std::string> m_stat_invalid_definition;
    std::unordered_set<std::string> m_stat_unrecognized_node_class;
    std::unordered_set<std::string> m_stat_unrecognized_node_definition;
    std::unordered_set<std::string> m_stat_python_modules;
};

} // namespace data
