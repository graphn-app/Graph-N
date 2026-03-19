
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
    static bool deserialize(Workflow* wf, const std::string& json);

private:
    static void serialize_node(const NodeInstance* node, nlohmann::json& json);
    static NodeInstance* deserialize_node(Workflow* wf, NodeInstance* node, const nlohmann::json& json);
};

} // namespace data
