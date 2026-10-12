#include "ui/custom_nodes/custom_js_node.h"
#include "core/workflow/custom_nodes/cni_fast_action_rgthree.h"
#include "core/managers/workflows_manager.h"

namespace ui
{

void CustomJsNode::draw_FastActionsButtonRgthree(data::NodeInstance* node, float off, float width)
{
    ImGui::Spacing();
    ImGui::SetCursorPosX(off);
    if (ImGui::Button(ICON_LC_CLAPPERBOARD " Action!", ImVec2(width, 0)))
    {
        LogRAssert(node->has_object() && node->obj_flags().has(NodeObjFlags::SpecFastActRgthree), "Not an action node", );
        LogRAssert(!node->get_inputs().empty(), "Not input pins", );

        auto wf = g::pWfsManager->access(node->get_workflow_id());
        wf->handle_rgthree_switchers(node->get_inputs().front());
    }
}

} // namespace ui
