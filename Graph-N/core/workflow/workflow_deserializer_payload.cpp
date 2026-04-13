
#include "core/workflow/workflow_serializer_to_json.h"

namespace data
{

bool WorkflowSerializerToJson::deserialize_all_wf_names(const std::string& payload, std::list<std::string>& names)
{
    try
    {
        auto j = o_json::parse(payload);
        if (!j.is_array())
            return false;

        for (auto& jitem : j)
        {
            if (jitem.contains("path") && jitem["path"].is_string())
                names.push_back(jitem["path"]);
        }

        return true;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::API, ex.what());
    }
    return false;
}
} // namespace data
