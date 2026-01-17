
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

class PanelWfList final : public PanelBase<PanelWfListData>
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
    void show_cfg_buttons();
    void show_wfs_list();
    void add_new_wf();
    void rename_wf();
    void locate_wf();
    void sort_wfs();
    void delete_wf();
    void view_deleted();

private:
    std::list<data::Workflow*> m_wfs;
    data::Workflow* m_active = nullptr;
    bool m_active_updated = false;
};


} // namespace ui
