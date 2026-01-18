
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelOutput final : 
    public PanelEmptyWithDescr,
    public PanelBase<PanelCommonData>
{
public:
    PanelOutput();
    ~PanelOutput() override;

    void init() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
