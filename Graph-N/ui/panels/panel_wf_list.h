
#pragma once
#include "ui/panels/panel_base.h"

namespace data
{
class Workflow;
}

namespace ui
{

class PanelWfList final : public PanelBase<PanelCommonData>
{
public:
    PanelWfList();
    ~PanelWfList() override;

    void init() override;
    void stop_all_threads() override;

    void add_wf(data::Workflow* wf);
    void remove_wf(data::Workflow* wf);

private:
    DlgCode show(bool& focused) override;
    void show_wfs();

private:
    std::list<data::Workflow*> m_wfs;
};


} // namespace ui
