
#include "ui/panels/panel_logs.h"
#include "ui/graph-n_flow.h"
#include "ui/helpers/font_guard.h"
#include "main.h"
#include "core/managers/comfyui_manager.h"
#include "core/managers/settings_manager.h"

namespace ui
{

PanelServerLogs::PanelServerLogs()
    : PanelBase(DlgObj::PanelServerLogs, ICON_LC_TERMINAL " Server Logs", ICON_LC_TERMINAL, DlgObjID[cast::to_szt(DlgObj::PanelServerLogs)])
{}

PanelServerLogs::~PanelServerLogs()
{
    LogF;
}

void PanelServerLogs::init()
{
    LogF;
    PanelBase::init();

    m_clr_hover = g::pOptsManager->color_load(AppColor::SelectedItem);

    ImVec4 c = ImGui::ColorConvertU32ToFloat4(m_clr_hover);
    c.x = std::min(c.x * 0.8f, 1.0f); // R
    c.y = std::min(c.y * 0.8f, 1.0f); // G
    c.z = std::min(c.z * 0.8f, 1.0f); // B
    m_clr_pressed = ImGui::ColorConvertFloat4ToU32(c);

    create_toolbar();
}

void PanelServerLogs::stop_all_threads()
{
    LogF;
}

DlgCode PanelServerLogs::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        if (false) //!g::pComfyUIManager->get_process_started())
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Server Log.",
                {},
                {
                    "Real-time stream of the ComfyUI console output.",
                    "Useful for debugging generation errors and monitoring model loading status.",
                    "Logs capturing is disabled or Server is not running.",
                    "Enable in Settings > " APP_COMFYUI
                },
                "Open settings",
                []()
                {
                    g::pFlow->open_settings_dlg(OptPane::LocalServer);
                }
            );
        }
        else
        {
            draw_toolbar();
            show_logs();
        }
    }
    return end_panel(opened);
}

void PanelServerLogs::create_toolbar()
{
    clear_toolbar();

    // left part
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::InputHint;
        el.buffer = &m_search;
        el.hint = "Search logs";
        el.id = "##searchlogs";
        el.input_w = 0.5f;
        add_toolbar_element(std::move(el), true);
    } 
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_CHEVRON_UP;
        el.tooltip = "Search up";
        el.on_click = std::bind(&PanelServerLogs::on_search_up, this, std::placeholders::_1);
        el.on_preview = std::bind(&PanelServerLogs::on_pre_search, this, std::placeholders::_1);
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_CHEVRON_DOWN;
        el.tooltip = "Search down";
        el.on_click = std::bind(&PanelServerLogs::on_search_down, this, std::placeholders::_1);
        el.on_preview = std::bind(&PanelServerLogs::on_pre_search, this, std::placeholders::_1);
        add_toolbar_element(std::move(el), true);
    }

    // right part
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_CHEVRONS_DOWN;
        el.tooltip = "Toggle autoscroll";
        el.toggler = true;
        el.toggled = m_autoscroll;
        el.on_click = std::bind(&PanelServerLogs::on_autoscroll, this, std::placeholders::_1);
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_X;
        el.tooltip = "Clear logs";
        el.on_click = std::bind(&PanelServerLogs::on_clear, this, std::placeholders::_1);
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Empty;
        add_toolbar_element(std::move(el), false);
    }

    finalize_toolbar();
}
void PanelServerLogs::on_pre_search(PanelToolbar::Element& el)
{
    el.enabled = !m_search.empty();
}

void PanelServerLogs::on_search_up(PanelToolbar::Element& /*el*/)
{}

void PanelServerLogs::on_search_down(PanelToolbar::Element& /*el*/)
{}

void PanelServerLogs::on_autoscroll(PanelToolbar::Element& /*el*/)
{
    m_autoscroll = !m_autoscroll;
}

void PanelServerLogs::on_clear(PanelToolbar::Element& /*el*/)
{}

void PanelServerLogs::show_logs()
{
    g::pComfyUIManager->append_log(m_logs);
}

} // namespace ui
