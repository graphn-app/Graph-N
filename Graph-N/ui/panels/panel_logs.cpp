#include "ui/panels/panel_logs.h"
#include "ui/graph-n_flow.h"
#include "ui/helpers/font_guard.h"
#include "core/managers/comfyui_manager.h"
#include "core/managers/settings_manager.h"
#include "main.h"

namespace ui
{

PanelServerLogs::PanelServerLogs()
    : PanelBase(DlgObj::PanelServerLogs, "Server Logs")
{}

PanelServerLogs::~PanelServerLogs() noexcept = default;

void PanelServerLogs::init()
{
    PanelBase::init();

    m_clr_hover = g::pOptsManager->color_load(AppColor::SelectedItem);

    ImVec4 c = ImGui::ColorConvertU32ToFloat4(m_clr_hover);
    c.x = std::min(c.x * 0.8f, 1.0f); // R
    c.y = std::min(c.y * 0.8f, 1.0f); // G
    c.z = std::min(c.z * 0.8f, 1.0f); // B
    m_clr_pressed = ImGui::ColorConvertFloat4ToU32(c);

    create_toolbar();
}

DlgCode PanelServerLogs::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        if (!g::pOptsManager->comfyui_capture_output_load() && m_logs.empty())
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
        else [[likely]]
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
        el.buffer_str = &m_search;
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
        el.on_click = [this](PanelToolbar::Element& el) { on_search_up(el); };
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_search(el); };
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_CHEVRON_DOWN;
        el.tooltip = "Search down";
        el.on_click = [this](PanelToolbar::Element& el) { on_search_down(el); };
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_search(el); };
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
        el.on_click = [this](PanelToolbar::Element& el) noexcept { on_autoscroll(el); };
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_X;
        el.tooltip = "Clear logs";
        el.on_click = [this](PanelToolbar::Element& el) noexcept { on_clear(el); };
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Empty;
        add_toolbar_element(std::move(el), false);
    }

    finalize_toolbar();
}

void PanelServerLogs::on_pre_search(PanelToolbar::Element& el) noexcept
{
    el.enabled = !m_search.empty();
}

void PanelServerLogs::on_search_up(PanelToolbar::Element& /*el*/)
{
    if (m_logs.empty())
        return;
    const int32_t start_from = (-1 == m_found_index ? cast::to_i32(m_logs.size()) - 1 : m_found_index - 1);
    if (start_from < 0)
        return;

    const auto search = tools::str::to_lower_ascii(m_search);
    for (int32_t idx = start_from; idx >= 0; idx--)
    {
        const auto line = tools::str::to_lower_ascii(m_logs[idx]);
        if (line.find(search) != std::string::npos)
        {
            m_found_index = idx;
            m_scroll_to_found = true;
            return;
        }
    }

    m_found_index = -1;
}

void PanelServerLogs::on_search_down(PanelToolbar::Element& /*el*/)
{
    if (m_logs.empty())
        return;
    const int32_t start_from = (-1 == m_found_index ? 0 : m_found_index + 1);
    if (start_from >= m_logs.size())
        return;

    const auto search = tools::str::to_lower_ascii(m_search);
    for (int32_t idx = start_from; idx < m_logs.size(); idx++)
    {
        const auto line = tools::str::to_lower_ascii(m_logs[idx]);
        if (line.find(search) != std::string::npos)
        {
            m_found_index = idx;
            m_scroll_to_found = true;
            return;
        }
    }

    m_found_index = -1;
}

void PanelServerLogs::on_autoscroll(PanelToolbar::Element& /*el*/) noexcept
{
    m_autoscroll = !m_autoscroll;
}

void PanelServerLogs::on_clear(PanelToolbar::Element& /*el*/) noexcept
{
    decltype(m_logs) empty;
    empty.swap(m_logs);
    m_found_index = -1;
}

bool PanelServerLogs::update_logs()
{
    const auto old_size = m_logs.size();
    g::pComfyUIManager->append_log(m_logs);
    const auto new_size = m_logs.size();
    search_for_address(old_size, new_size);

    const size_t keep = g::pOptsManager->log_comfyui_log_in_ram_load();
    while (m_logs.size() > keep)
    {
        m_logs.pop_front();
    }

    return old_size != new_size;
}

void PanelServerLogs::search_for_address(size_t from, size_t to)
{
    if (!m_host.empty() || g::pFlow->get_online() || !g::pOptsManager->comfyui_parse_address_load())
        return;

    constexpr const std::string_view key = "To see the GUI go to";
    constexpr const std::string_view key_address = "http://";

    for (size_t idx = from; idx < to; idx++)
    {
        const auto& line = m_logs[idx];
        const auto key_pos = line.find(key);
        if (key_pos != std::string::npos)
        {
            const auto host_pos = line.find(key_address, key_pos + key.size());
            if (host_pos != std::string::npos)
            {
                const auto port_pos = line.find(":", host_pos + key_address.size());
                if (host_pos != std::string::npos)
                {
                    m_host = line.substr(host_pos, port_pos - host_pos);
                    m_port = strtoll(line.c_str() + port_pos + 1, nullptr, 10);
                    Log("Address found: " << m_host << ":" << m_port);
                    break;
                }                
            }
        }
    }
}

void PanelServerLogs::show_logs()
{
    const bool scroll_to_bottom = update_logs() && m_autoscroll;

    ImGui::Spacing();
    if (ImGui::BeginChild("ServerLogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGuiListClipper clipper;
        clipper.Begin(m_logs.empty() ? 1 : cast::to_i32(m_logs.size()) + 2);

        while (clipper.Step())
        {
            for (int32_t line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
                if (0 == line_no)
                {
                    auto& path = g::pComfyUIManager->get_log_filepath();
                    if (path.empty())
                        ImGui::Selectable("##stub", false);
                    else
                    {
                        if (ImGuiEx::UrlButton(tools::str::path_to_string(path).c_str(), "Open log file"))
                        {
                            tools::path::open_file(g::pComfyUIManager->get_log_filepath());
                        }
                    }
                    continue;
                }

                if (line_no >= m_logs.size())
                {
                    ImGui::Selectable("##empty", false);
                    continue;
                }

                if (m_found_index == line_no - 1)
                    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));

                ImGui::PushID(line_no);
                const std::string& line_text = m_logs[line_no - 1];
                if (ImGui::Selectable(line_text.c_str(), false) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    ImGui::SetClipboardText(line_text.c_str());
                    m_toast = ImGuiEx::toast_prepare("Line copied", g::pOptsManager->color_load(AppColor::SelectedItem));
                }
                ImGui::PopID();

                if (m_found_index == line_no - 1)
                    ImGui::PopStyleColor();
            }
        }

        if (m_found_index >= 0 && m_scroll_to_found)
        {
            const float scroll_y = (m_found_index * ImGui::GetTextLineHeightWithSpacing()) - (ImGui::GetWindowHeight() / 2.0f);
            ImGui::SetScrollY(std::max(0.0f, scroll_y));
            m_scroll_to_found = false;
        }
        else if (scroll_to_bottom || (m_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGuiEx::toast_show(m_toast);
    }
    ImGui::EndChild();
}

} // namespace ui
