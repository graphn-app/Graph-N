
#include "core/workflow/workflow_serializer_to_json.h"

namespace data
{

bool WorkflowSerializerToJson::deserialize_all_wf_names(const std::string& payload, std::list<std::pair<std::string, tools::dt>>& projects)
{
    try
    {
        const auto j = u_json::parse(payload);
        if (!j.is_array())
            return false;

        for (auto& jitem : j)
        {
            std::string name;
            if (jitem.contains("path") && jitem["path"].is_string())
                name = jitem["path"].get<std::string>();

            tools::dt time;
            if (jitem.contains("modified") && jitem["modified"].is_number())
            {
                if (jitem["modified"].is_number_float())
                {
                    uint64_t t = uint64_t(jitem["modified"].get<double>());
                    time = tools::dt::from_time(t);
                }
                else
                {
                    uint64_t t = jitem["modified"].get<int64_t>();
                    time = tools::dt::from_time(t);
                }
            }

            projects.push_back({ name, time });
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
