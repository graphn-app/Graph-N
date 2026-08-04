#include "core/workflow/serializer/wf_deserializer_from_json.h"

namespace data
{

bool WfDeserializerFromJson::deserialize_all_wf_names(const std::string& payload, std::list<std::pair<std::string, tools::dt>>& projects)
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
                    const time_t t = cast::to_time(jitem["modified"].get<double>());
                    time = tools::dt::from_time(t);
                }
                else
                {
                    const time_t t = cast::to_time(jitem["modified"].get<int64_t>());
                    time = tools::dt::from_time(t);
                }
            }

            projects.push_back({ name, time });
        }

        return true;
    }
    catch (const std::exception& ex)
    {
        LogExcepS("Wf json all names parse ex:" << ex.what());
    }
    return false;
}
} // namespace data
