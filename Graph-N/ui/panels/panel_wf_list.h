
#pragma once
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
    void stop_all_threads() override;

    void add_wf(data::Workflow* wf);
    void remove_wf(data::Workflow* wf);
    void remove_all_wf() { m_wfs.clear(); }
    void set_active(data::Workflow* wf);

private:
    DlgCode show(bool& focused) override;
    void show_wfs();

    void rebuild_items();
    void handle_dirty(const PanelItemState& dirty_item);

    void on_add_new_wf(PanelToolbar::Button& button);
    void on_rename_wf(PanelToolbar::Button& button);
    void on_pre_rename_wf(PanelToolbar::Button& button);
    void on_locate_wf(PanelToolbar::Button& button);
    void on_pre_locate_wf(PanelToolbar::Button& button);
    void on_sort_wfs(PanelToolbar::Button& button);
    void on_delete_wf(PanelToolbar::Button& button);
    void on_pre_delete_wf(PanelToolbar::Button& button);
    void on_view_deleted(PanelToolbar::Button& button);

private:
    std::list<data::Workflow*> m_wfs;
    data::Workflow* m_active = nullptr;
    bool m_active_updated = false;
    std::vector<PanelListItem> m_panel_items;
};


} // namespace ui
