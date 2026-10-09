#include "core/workflow/serializer/wf_serializer_to_json_api.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node.h"

namespace data
{

o_json WfSerializerToJsonApi::serialize_custom_node(data::NodeInstance* node, int32_t& /*input_count*/)
{
    switch (node->get_object()->get_hash())
    {
    case js::LoadImageFromUrl.hash:
        break;
    }
    return {};
}

} // namespace data
