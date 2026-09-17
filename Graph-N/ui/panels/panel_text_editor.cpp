#include "ui/panels/panel_text_editor.h"
#include "core/workflow/node.h"
#include "core/workflow/pin.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow_action.h"
#include "core/managers/workflows_manager.h"

namespace ui
{

PanelTextEditor::PanelTextEditor(const char* panel_id, bool is_def /*= false*/)
    : PanelBase(DlgObj::PanelTextEditor, "Editor", panel_id)
    , m_default(is_def)
{}

PanelTextEditor::~PanelTextEditor() noexcept = default;

void PanelTextEditor::free() noexcept
{
    reset();
}

void PanelTextEditor::update(wid wf, data::NodeInstance* node, data::PinInstance* pin, std::string* buffer, const char* hint)
{
    LogRAssert(node && buffer, "Zero ptr", );
    clear_toolbar();
    m_current = nullptr;
    m_editing = nullptr;
    m_was_edited = nullptr;

    for (auto& ed : m_editors)
    {
        if (ed.wf == wf && ed.node == node && ed.pin == pin)
        {
            ed.buffer = buffer;
            ed.hint = hint;
            ed.action = g::pWfsManager->access(ed.wf)->action();
            ed.ro = (pin != nullptr);
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
        ed.ro = (pin != nullptr);
        ed.action = g::pWfsManager->access(ed.wf)->action();
        m_editors.push_back(ed);
        m_current = &m_editors.back();
    }

    if (pin != nullptr)
        set_name(node->get_name() + " | Editor");
    else
        set_name(node->get_name() + " | Viewer");
    m_need_focus = true;
    create_toolbar();
}

void PanelTextEditor::update_node_name(wid wf, data::NodeInstance* node)
{
    for (auto& ed : m_editors)
    {
        if (ed.wf == wf && ed.node == node)
        {
            if (!ed.ro)
                set_name(node->get_name() + " | Editor");
            else
                set_name(node->get_name() + " | Viewer");
        }
    }

    create_toolbar();
}

bool PanelTextEditor::activate(wid wf)
{
    clear_toolbar();
    m_current = nullptr;
    m_editing = nullptr;
    m_was_edited = nullptr;

    for (auto& ed : m_editors)
    {
        if (ed.wf == wf)
        {
            m_current = &ed;
            update_node_name(m_current->wf, m_current->node);
            return true;
        }
    }

    return false;
}

void PanelTextEditor::reset() noexcept
{
    clear_toolbar();
    m_current = nullptr;
    m_editing = nullptr;
    m_was_edited = nullptr;
    m_old_pin_val = {};

    m_editors.clear();
    m_pinned = false;
}

void PanelTextEditor::reset(wid wf, const data::NodeInstance* node /*= nullptr*/, const data::PinInstance* pin /*= nullptr*/) noexcept
{
    for (auto it = m_editors.begin(); it != m_editors.end(); )
    {
        if (it->wf == wf &&
            (it->node == node || nullptr == node) &&
            (it->pin == pin || nullptr == pin))
        {
            if (m_current == &*it)
            {
                clear_toolbar();
                m_pinned = false;
                m_current = nullptr;
                m_editing = nullptr;
                m_was_edited = nullptr;
                m_old_pin_val = {};
            }
            it = m_editors.erase(it);
        }
        else
            ++it;
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

void PanelTextEditor::unpin() noexcept
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
        el.on_click = [this](PanelToolbar::Element& el) { on_pin(el); };
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_pin(el); };
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_SEARCH;
        el.tooltip = "Locate node on graph";
        el.on_click = [this](PanelToolbar::Element& el) { on_locate(el); };
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Text;
        if (m_current != nullptr)
            el.text = m_current->node->get_name().c_str();
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
        if (m_current != nullptr && m_current->pin != nullptr)
            el.text = m_current->pin->state().name.c_str();
        else
            el.text = "[output]";
        el.text_bold = true;
        el.text_clr = g::pOptsManager->color_load(AppColor::TextEditorCaption2);
        add_toolbar_element(std::move(el), true);
    }

    finalize_toolbar(false /*separator*/);
    set_use_regular_size_icons();
}

void PanelTextEditor::on_pre_pin(PanelToolbar::Element& el) noexcept
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
            draw_toolbar();

            ImGui::PushID(m_current->pin);
            if (ImGui::BeginChild("TextEditorChild", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
            {
                if (m_need_focus)
                {
                    ImGui::SetKeyboardFocusHere();
                    m_need_focus = false;
                }
                ImGuiEx::InputTextMultilineWithHint("##TextEditorLabel", m_current->hint, m_current->buffer, ImVec2(-FLT_MIN, -FLT_MIN),
                    ImGuiInputTextFlags_AllowTabInput | (m_current->ro ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None));
                if (m_current->pin != nullptr)
                    handle_text_editing();
            }
            ImGui::EndChild();
            ImGui::PopID();
        }
    }
    return end_panel(opened);
}

void PanelTextEditor::handle_text_editing()
{
    if (ImGui::IsItemActivated())
        m_old_pin_val = m_current->pin->state().value;

    const bool active_now = ImGui::IsItemActive();

    if (ImGui::IsItemEdited())
        m_was_edited = m_current->pin;

    if (m_editing == m_current->pin && !active_now)
    {
        if (m_was_edited == m_current->pin)
        {
            m_current->action->set_pin_val(m_current->pin, std::move(m_old_pin_val));
            m_was_edited = nullptr;
        }
    }

    if (active_now)
        m_editing = m_current->pin;
}

} // namespace ui
