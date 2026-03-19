
#include "core/workflow/workflow_serializer_to_json.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

std::string WorkflowSerializerToJson::serialize(const Workflow* /*wf*/)
{
    return {};
}

void WorkflowSerializerToJson::serialize_node(const NodeInstance* /*node*/, nlohmann::json& /*json*/)
{
}

bool WorkflowSerializerToJson::deserialize(Workflow* /*wf*/, const std::string& /*json*/)
{
    try
    {
        return true;
    }
    catch (std::exception& ex)
    {
        // TODO remove wf
        // handle errors if node/pin not found
        LogExcepS(ExcepCategory::Logic, ex.what());
    }
    return false;
}

NodeInstance* WorkflowSerializerToJson::deserialize_node(Workflow* /*wf*/, NodeInstance* /*node*/, const nlohmann::json& /*json*/)
{
    return nullptr;
}

} // namespace data
