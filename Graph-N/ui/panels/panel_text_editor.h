/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/panels/panel_base.h"

namespace data
{
class NodeInstance;
class PinInstance;
class WorkflowAction;
}

namespace ui
{

class PanelTextEditor final
    : public PanelBase<PanelCommonData>
    , public PanelToolbar
{
public:
    PanelTextEditor(const char* panel_id, bool is_def = false);
    ~PanelTextEditor() noexcept override;

    void stop_and_cleanup() noexcept override;

    bool is_pinned() const noexcept { return m_pinned; }
    bool is_default() const noexcept { return m_default; }
    bool is_active() const noexcept { return m_current != nullptr && m_current->wf != 0; }

    std::pair<int32_t, int32_t> get_text_selection() const noexcept { return std::make_pair(m_selection_begin, m_selection_end); }

    bool is_current(wid wf) const noexcept { return m_current != nullptr && m_current->wf == wf; }
    bool is_current(wid wf, const data::NodeInstance* node) const noexcept { return m_current != nullptr && m_current->wf == wf && m_current->node == node; }
    bool is_current(wid wf, const data::NodeInstance* node, const data::PinInstance* pin) const noexcept { return m_current != nullptr && m_current->wf == wf && m_current->node == node && m_current->pin == pin; }

    void update(wid wf, data::NodeInstance* node, data::PinInstance* pin, std::string* buffer, const char* hint);
    void update_node_name(wid wf, data::NodeInstance* node);

    bool activate(wid wf);

    void reset() noexcept;
    void reset(wid wf, const data::NodeInstance* node = nullptr, const data::PinInstance* pin = nullptr) noexcept;

    //bool get_is_empty() const { return m_wf != 0; }

private:
    DlgCode show(bool& focused) override;
    void create_toolbar();
    void pin();
    void unpin() noexcept;
    void handle_text_editing();

private:
    ImGuiID m_dock = 0;
    ImRect m_rect = {};
    bool m_change_pos = false;

    bool m_pinned = false;
    bool m_default = false;
    bool m_need_focus = false;

    struct Editor final
    {
        wid wf = 0;
        data::NodeInstance* node = nullptr;
        data::PinInstance* pin = nullptr;
        const char* hint = nullptr;
        std::string* buffer = nullptr;
        data::WorkflowAction* action = nullptr;
        bool ro = false;
    };
    Editor* m_current = nullptr;
    std::list<Editor> m_editors;

    Primitive m_old_pin_val;
    data::PinInstance* m_editing = nullptr;
    data::PinInstance* m_was_edited = nullptr;
    int32_t m_selection_begin = 0;
    int32_t m_selection_end = 0;
};


} // namespace ui
