
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

    static std::string serialize(const Workflow* wf);
    static std::string serialize_to_api(const Workflow* wf);

    static bool deserialize(Workflow* wf, const o_json& j);
    static bool deserialize_api(Workflow* wf, const o_json& j);

private:
    using LinkMap = std::map<iid, std::map<int32_t, iid>>;

    static iid serialize_nodes(const Workflow* wf, o_json& j);
    static void serialize_pins(const Workflow* wf, const NodeInstance* node, o_json& j);
    static void serialize_widgets(const NodeInstance* node, o_json& j);
    static iid serialize_links(const Workflow* wf, o_json& j);
    static void serialize_groups(const Workflow* wf, o_json& j);

    static void serialize_node_api(const NodeInstance* node, o_json& j);

    static NodeInstance* deserialize_node(Workflow* wf, const o_json& j, std::string& node_id_from_json, LinkMap& in_pins, LinkMap& out_pins, bool dummy);
    static bool deserialize_links(Workflow* wf, const o_json& j, const std::map<std::string, iid>& node_ids, const LinkMap& in_pins, const LinkMap& out_pins);
    static bool deserialize_groups(Workflow* wf, const o_json& j);

    static bool deserialize_node_api(NodeInstance* node, const o_json& j, std::string& node_id_from_json);
    static bool build_api_links(Workflow* wf);
};

} // namespace data
