/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_js_node.h"
#include "core/workflow/custom_nodes/cni_fast_action_rgthree.h"
#include "core/managers/workflows_manager.h"

namespace ui
{

void CustomJsNode::draw_CustomCombo(data::NodeInstance* /*node*/, float /*off*/, float /*width*/)
{
    // TODO
    //LogRAssert(node->m_js_data, "Data not initialized", );
    //auto data = cast::strict<js::CustomCombo_Data>(node->m_js_data.get());
    //LogRAssert(data, "Wrong data type", );
}

} // namespace ui
