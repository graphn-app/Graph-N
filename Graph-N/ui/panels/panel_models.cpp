
#include "ui/panels/panel_models.h"
#include "core/managers/nodes_manager.h"

namespace ui
{

PanelModels::PanelModels()
    : PanelBase(DlgObj::PanelModels, ICON_LC_COMPONENT " Model Library", ICON_LC_COMPONENT, "204150D3786A4F1D9DC9FDC6A9A3A3E9")
{}

PanelModels::~PanelModels()
{
    LogF;
}

void PanelModels::init()
{
    LogF;
    PanelBase::init();

}

void PanelModels::stop_all_threads()
{
    LogF;
}

DlgCode PanelModels::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        if (g::pNodesManager->get_models_tree().empty())
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Model Library.",
                {},
                {
                    "Displays available Checkpoints,",
                    "LoRAs, VAEs, and ControlNet models.",
                    "Make sure " APP_CAPTION " is connected",
                    "to the " APP_COMFYUI " server and",
                    "model paths are configured correctly.",
                    "Drag and drop models directly onto",
                    "the graph to quickly create loader nodes."
                }                
            );
            return end_panel(true);
        }

    }
    return end_panel(opened);
}

} // namespace ui
