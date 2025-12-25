
#include "ui/panels/panel_nodes.h"

namespace ui
{

PanelNodes::PanelNodes()
    : PanelBase(VisibleObject::PanelNodes)
{}

void PanelNodes::init()
{
    PanelBase::init();
    data()->class_id = PanelClassID::left;
}

DlgCode PanelNodes::show(bool& focused)
{
    if (!begin_panel("Node Library", focused))
        return end_panel(false);

    ImGui::Text("Nodes tree here");

    return end_panel(true);
}

} // namespace ui