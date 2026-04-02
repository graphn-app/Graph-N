
#include "core/workflow/workflow_serializer_to_json.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

bool WorkflowSerializerToJson::deserialize_api(Workflow* /*wf*/, const nlohmann::json& /*j*/)
{
    try
    {

        return false;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::API, ex.what());
    }
    return false;
}

bool WorkflowSerializerToJson::deserialize_node_api(NodeInstance* /*node*/, const nlohmann::json& /*j*/, std::string& /*node_id_from_json*/)
{
    try
    {

        return false;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::API, ex.what());
    }
    return false;
}

bool WorkflowSerializerToJson::build_api_links(Workflow* /*wf*/)
{
    return false;
}

} // namespace data
