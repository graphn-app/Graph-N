
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

    static bool deserialize(Workflow* wf, const nlohmann::json& json);
    static bool deserialize_api(Workflow* wf, const nlohmann::json& json);

private:
    using LinkMap = std::map<iid, std::map<int32_t, iid>>;

    static void serialize_node(const NodeInstance* node, nlohmann::json& json);
    static void serialize_node_api(const NodeInstance* node, nlohmann::json& json);

    static NodeInstance* deserialize_node(Workflow* wf, const nlohmann::json& json, std::string& node_id_from_json, LinkMap& in_pins, LinkMap& out_pins);
    static bool deserialize_links(Workflow* wf, const nlohmann::json& json, const std::map<std::string, iid>& node_ids, const LinkMap& in_pins, const LinkMap& out_pins);
    static bool deserialize_groups(Workflow* wf, const nlohmann::json& json);

    static bool deserialize_node_api(NodeInstance* node, const nlohmann::json& json, std::string& node_id_from_json);
    static bool build_api_links(Workflow* wf);

    static ImU32 parse_color(const std::string& color);
};

} // namespace data
