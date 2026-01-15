
#pragma once
#include "ui/panels/panel_base.h"

namespace data
{
class Workflow;
}

namespace ui
{

class PanelWorkflow final : public PanelBase<PanelCommonData>
{
public:
    PanelWorkflow(data::Workflow* wf, const char* name, const char* id);
    ~PanelWorkflow() override;

    void init() override;
    void stop_all_threads() override;

    data::Workflow* get_workflow() const { return m_wf; }

private:
    DlgCode show(bool& focused) override;

private:
    data::Workflow* m_wf = nullptr;
};


} // namespace ui
