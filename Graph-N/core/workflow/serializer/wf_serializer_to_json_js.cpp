#include "core/workflow/serializer/wf_serializer_to_json.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node.h"

namespace data
{

void WfSerializerToJson::serialize_js_node(o_json& /*j*/)
{
    switch (m_curr_node->get_object()->get_hash())
    {
    case js::LoadImageFromUrl.hash:
        break;
    }
}

} // namespace data
