/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/serializer/wf_deserializer_from_json.h"

namespace data
{

NodeInstance* WfDeserializerFromJson::deserialize_js_node(NodeInstance* /*node*/, const o_json& /*j*/)
{
    return nullptr;
}

} // namespace data
