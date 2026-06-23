
#pragma once

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;

class WorkflowSerializerToJson
{
    NO_COPY_MOVE_CTORS(WorkflowSerializerToJson);
public:

    static o_json serialize(const Workflow* wf);
    static o_json serialize_to_api(const Workflow* wf);

    static bool deserialize(Workflow* wf, const o_json& j);
    static bool deserialize_api(Workflow* wf, const o_json& j);

    static bool deserialize_all_wf_names(const std::string& payload, std::list<std::pair<std::string, tools::dt>>& projects);

private:
    using LinkMap = std::map<iid, std::map<int32_t, iid>>;  // node iid -> [pin index -> pin iid]
    using SubgraphMap = std::map<std::string, std::pair<std::string, ImRect>>; // subgraph uuid -> [subgraph id as node, subgraph pos]

    static iid serialize_nodes(const Workflow* wf, o_json& j);
    static void serialize_pins(const Workflow* wf, const NodeInstance* node, o_json& j);
    static void serialize_widgets(const NodeInstance* node, o_json& j);
    static iid serialize_links(const Workflow* wf, o_json& j);
    static void serialize_groups(const Workflow* wf, o_json& j);

    static NodeInstance* deserialize_node(Workflow* wf, const o_json& j, std::string& node_id_from_json, SubgraphMap& subgraphs, LinkMap& in_pins, LinkMap& out_pins, bool dummy);
    static bool deserialize_subgraph(Workflow* wf, const o_json& j, std::map<std::string, iid>& node_ids, SubgraphMap& subgraphs, LinkMap& in_pins, LinkMap& out_pins);
    static bool deserialize_links(Workflow* wf, const o_json& j, const std::map<std::string, iid>& node_ids, SubgraphMap& subgraphs, const LinkMap& in_pins, const LinkMap& out_pins);
    static NodeInstance* deserialize_group(Workflow* wf, const o_json& j);

    struct ApiNode
    {
        std::string node_id;
        iid node_iid = 0;
        std::string class_type;
        std::string title;

        struct Input
        {
            std::string pin_name;
            iid pin_iid = 0;
            bool is_linked = true;
            primitive raw_input;
            std::pair<std::string, int32_t> linked_input; // source node, source pin index
        };
        std::list<Input> inputs;
    };

    static NodeInstance* create_api_node(Workflow* wf, ApiNode& api_node, bool dummy);
    static void link_api_nodes(Workflow* wf, const std::list<ApiNode>& api_nodes);
};

} // namespace data
