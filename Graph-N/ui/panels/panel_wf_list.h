
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelWfList final : public PanelBase<PanelCommonData>
{
public:
    PanelWfList();
    ~PanelWfList() override;

    void init() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
