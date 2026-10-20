/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/panels/panel_base.h"
#include "core/listeners.h"

namespace data
{
class Workflow;
}

namespace ui
{

struct PanelWfListData final : public PanelCommonData
{
    ImU32 clr_active = 0;
    ImU32 clr_hover_btn = 0;
    ImU32 clr_active_btn = 0;
};

class PanelWfList final
    : public PanelEmptyWithDescr
    , public PanelWithList
    , public PanelToolbar
    , public PanelBase<PanelWfListData>
    , public listener::HotkeysChangedHandler
{
public:
    PanelWfList();
    ~PanelWfList() noexcept override;

    void init() override;
    void stop_and_cleanup() noexcept override;

    void rebuild_items();
    void rename_project(wid id) noexcept;

private:
    DlgCode show(bool& focused) override;
    void show_wfs();

    void create_toolbar();
    void handle_hotkeys_changed(); // HotkeysChangedHandler

    void rebuild_labels();
    void rebuild_trash();
    void handle_dirty(const PanelItemEvent& ev);
    void handle_dirty_labels(const PanelItemEvent& ev);
    void handle_dirty_trash(const PanelItemEvent& ev);
    void draw_popups();

    void on_deleted_mode(PanelToolbar::Element& el);
    void on_labels_mode(PanelToolbar::Element& el);
    void on_show_tt(int32_t idx) noexcept;

private:
    wid m_active = 0;
    std::vector<mngr::WfData*> m_wfs;
    std::vector<PanelListItem> m_panel_items;
    bool m_deleted_mode = false;
    bool m_labels_mode = false;
    uint32_t m_new_label = 0;
    PanelWithList::TtCallbacks m_tt_callbacks;
    std::set<uint32_t> m_curr_labels;

    void* m_active_popup_ptr = nullptr;
    std::set<uint32_t> m_active_popup_labels;
    char m_new_label_buff[31] = {};
    uint32_t m_active_popup_label_id = 0;
    wid m_active_wf_for_menu = 0;
};


} // namespace ui
