
#include "ui/panels/panel_text_editor.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/workflows_manager.h"

namespace ui
{

PanelTextEditor::PanelTextEditor(const char* panel_id, bool is_def /*= false*/)
    : PanelBase(DlgObj::PanelTextEditor, "Editor", panel_id)
    , m_default(is_def)
{}

PanelTextEditor::~PanelTextEditor()
{
    LogF;
}

void PanelTextEditor::init()
{
    LogF;
    PanelBase::init();

}

void PanelTextEditor::free()
{
    LogF;
    reset();
}

void PanelTextEditor::stop_all_threads()
{
    LogF;
}

void PanelTextEditor::update(wid wf, data::NodeInstance* node, data::PinInstance* pin, std::string* buffer, const char* hint)
{
    m_current = nullptr;
    for (auto& ed : m_editors)
    {
        if (ed.wf == wf && ed.node == node && ed.pin == pin)
        {
            ed.buffer = buffer;
            ed.hint = hint;
            m_current = &ed;
            break;
        }
    }

    if (nullptr == m_current)
    {
        Editor ed;
        ed.wf = wf;
        ed.node = node;
        ed.pin = pin;
        ed.buffer = buffer;
        ed.hint = hint;
        m_editors.push_back(ed);
        m_current = &m_editors.back();
        test_names_changed();
    }

    set_name(node->get_name() + " | Editor");
    m_need_focus = true;
}

void PanelTextEditor::test_names_changed()
{
    if (nullptr == m_current)
        return;

    if (m_current->node_name != m_current->node->get_name())
    {
        m_current->node_name = m_current->node->get_name();
        create_toolbar();
    }
}

bool PanelTextEditor::activate(wid wf)
{
    m_current = nullptr;
    for (auto& ed : m_editors)
    {
        if (ed.wf == wf)
        {
            m_current = &ed;
            return true;
        }
    }

    return false;
}

void PanelTextEditor::reset()
{
    m_current = nullptr;
    m_editors.clear();
    m_pinned = false;
}

void PanelTextEditor::reset(wid wf)
{
    for (auto it = m_editors.begin(); it != m_editors.end(); ++it)
    {
        if (it->wf == wf)
        {
            if (m_current == &*it)
            {
                m_pinned = false;
                m_current = nullptr;
            }
            m_editors.erase(it);
            return;
        }
    }
}

void PanelTextEditor::reset(wid wf, data::NodeInstance* node)
{
    for (auto it = m_editors.begin(); it != m_editors.end(); ++it)
    {
        if (it->wf == wf && it->node == node)
        {
            if (m_current == &*it)
            {
                m_pinned = false;
                m_current = nullptr;
            }
            m_editors.erase(it);
            return;
        }
    }
}

void PanelTextEditor::pin()
{
    if (nullptr == m_current)
        return;

    m_pinned = true;
    m_default = false;

    for (auto it = m_editors.begin(); it != m_editors.end(); )
    {
        if (m_current != &*it)
            it = m_editors.erase(it);
        else
            ++it;
    }

    m_dock = ImGui::GetCurrentWindow()->DockId;
    m_rect = ImGui::GetCurrentWindow()->Rect();
    m_change_pos = true;

    auto new_id = tools::str::generate_uuid_v4();
    Log("Panel pinned wf=" << m_current->wf << ", old_id=" << get_panel_id() << ", new id=" << new_id);
    set_id(new_id);
}

void PanelTextEditor::unpin()
{
    if (nullptr == m_current)
        return;

    m_pinned = false;
    Log("Panel unpinned wf=" << m_current->wf << ", id=" << get_panel_id());
}

void PanelTextEditor::create_toolbar()
{
    clear_toolbar();

    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Empty;
        add_toolbar_element(std::move(el), true);
    } 
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_PIN;
        el.tooltip = "Pin this editor to keep it open";
        el.toggler = true;
        el.on_click = std::bind(&PanelTextEditor::on_pin, this, std::placeholders::_1);
        el.on_preview = std::bind(&PanelTextEditor::on_pre_pin, this, std::placeholders::_1);
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_SEARCH;
        el.tooltip = "Locate node on graph";
        el.on_click = std::bind(&PanelTextEditor::on_locate, this, std::placeholders::_1);
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Text;
        if (m_current != nullptr)
            el.text = m_current->node->get_name();
        //el.text_bold = true;
        el.text_clr = g::pOptsManager->color_load(AppColor::TextEditorCaption1);
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Text;
        if (m_current != nullptr)
            el.text = ICON_LC_CHEVRON_RIGHT;
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Text;
        if (m_current != nullptr)
            el.text = m_current->pin->get_object()->name;
        el.text_bold = true;
        el.text_clr = g::pOptsManager->color_load(AppColor::TextEditorCaption2);
        add_toolbar_element(std::move(el), true);
    }

    finalize_toolbar();
    set_use_regular_size_icons();
}

void PanelTextEditor::on_pre_pin(PanelToolbar::Element& el)
{
    el.icon = (m_pinned ? ICON_LC_PIN_OFF : ICON_LC_PIN);
    el.tooltip = (!m_pinned ? "Pin this editor to keep it open" : "Unpin editor");
}

void PanelTextEditor::on_pin(PanelToolbar::Element& /*el*/)
{
    if (m_pinned)
        unpin();
    else
        pin();
}

void PanelTextEditor::on_locate(PanelToolbar::Element& /*el*/)
{
    if (m_current != nullptr)
        g::pFlow->wf_locate_node(m_current->wf, m_current->node);
}

void PanelTextEditor::show_toolbar()
{
    test_names_changed();
    draw_toolbar();
}

DlgCode PanelTextEditor::show(bool& focused)
{
    if (m_change_pos)
    {
        if (m_dock != 0)
            ImGui::SetNextWindowDockID(m_dock, ImGuiCond_FirstUseEver);
        else if (m_rect.GetSize().x > 0)
        {
            ImGui::SetNextWindowPos(ImVec2(m_rect.Min), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(m_rect.GetSize(), ImGuiCond_FirstUseEver);
        }
        m_change_pos = false;
    }

    bool opened = true;
    if (begin_panel(opened, focused, m_default) && opened)
    {
        if (m_current != nullptr && m_current->buffer != nullptr)
        {
            show_toolbar();

            ImGui::PushID(m_current->pin);
            if (ImGui::BeginChild("TextEditorChild", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
            {
                if (m_need_focus)
                {
                    ImGui::SetKeyboardFocusHere();
                    m_need_focus = false;
                }
                ImGuiEx::InputTextMultilineWithHint("##TextEditorLabel", m_current->hint, m_current->buffer, ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_AllowTabInput);
            }
            ImGui::EndChild();
            ImGui::PopID();
        }
    }
    return end_panel(opened);
}

} // namespace ui
