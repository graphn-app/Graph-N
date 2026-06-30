#pragma once
#include "core/base/structs.h"
#include "ui/panels/panel_base.h"

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

class PanelWfList final :
    public PanelEmptyWithDescr,
    public PanelWithList,
    public PanelToolbar,
    public PanelBase<PanelWfListData>
{
public:
    PanelWfList();
    ~PanelWfList() override;

    void init() override;
    void free() override;
    void stop_all_threads() override;

    void rebuild_items();
    void rename_project();

private:
    DlgCode show(bool& focused) override;
    void show_wfs();

    void create_toolbar();

    void rebuild_labels();
    void rebuild_trash();
    void handle_dirty(const PanelItemState& dirty_item);
    void handle_dirty_labels(const PanelItemState& dirty_item);
    void handle_dirty_trash(const PanelItemState& dirty_item);

    void on_add_new_wf(PanelToolbar::Element& el);
    void on_rename_wf(PanelToolbar::Element& el);
    void on_pre_rename_wf(PanelToolbar::Element& el);
    void on_copy_wf(PanelToolbar::Element& el);
    void on_pre_copy_wf(PanelToolbar::Element& el);
    void on_delete_wf(PanelToolbar::Element& el);
    void on_pre_delete_wf(PanelToolbar::Element& el);
    void on_deleted_mode(PanelToolbar::Element& el);
    void on_labels_mode(PanelToolbar::Element& el);
    void on_add_label(PanelToolbar::Element& el);
    void on_show_tt(int32_t idx);

private:
    wid m_active = 0;
    std::vector<mngr::WfData*> m_wfs;
    std::vector<PanelListItem> m_panel_items;
    bool m_deleted_mode = false;
    bool m_labels_mode = false;
    uint32_t m_new_label = 0;
    PanelWithList::TtCallbacks m_tt_callbacks;
};


} // namespace ui
