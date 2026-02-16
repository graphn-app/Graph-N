
#pragma once
#include "ui/panels/panel_base.h"

namespace data
{
class NodeInstance;
class PinInstance;
}

namespace ui
{

class PanelTextEditor final :
    public PanelBase<PanelCommonData>,
    public PanelToolbar
{
public:
    PanelTextEditor(const char* panel_id, bool is_def = false);
    ~PanelTextEditor() override;

    void init() override;
    void free() override;
    void stop_all_threads() override;

    inline bool is_pinned() const { return m_pinned; }
    inline bool is_default() const { return m_default; }
    inline bool is_active() const { return m_current != nullptr && m_current->wf != 0; }

    inline bool is_current(wid wf) const { return m_current != nullptr && m_current->wf == wf; }
    inline bool is_current(wid wf, data::NodeInstance* node) const { return m_current != nullptr && m_current->wf == wf && m_current->node == node; }
    inline bool is_current(wid wf, data::NodeInstance* node, data::PinInstance* pin) const { return m_current != nullptr && m_current->wf == wf && m_current->node == node && m_current->pin == pin; }

    void update(wid wf, data::NodeInstance* node, data::PinInstance* pin, std::string* buffer, const char* hint);

    bool activate(wid wf);

    void reset();
    void reset(wid wf);
    void reset(wid wf, data::NodeInstance* node);

    //inline bool get_is_empty() const { return m_wf != 0; }

private:
    DlgCode show(bool& focused) override;

    void create_toolbar();
    void show_toolbar();

    void pin();
    void unpin();

    void test_names_changed();

    void on_pre_pin(PanelToolbar::Element& el);
    void on_pin(PanelToolbar::Element& el);
    void on_locate(PanelToolbar::Element& el);

private:
    ImGuiID m_dock = 0;
    ImRect m_rect = {};
    bool m_change_pos = false;

    bool m_pinned = false;
    bool m_default = false;
    bool m_need_focus = false;

    struct Editor
    {
        wid wf = 0;
        data::NodeInstance* node = nullptr;
        std::string node_name;
        data::PinInstance* pin = nullptr;
        const char* hint = nullptr;
        std::string* buffer = nullptr;

    };
    Editor* m_current = nullptr;
    std::list<Editor> m_editors;
};


} // namespace ui
