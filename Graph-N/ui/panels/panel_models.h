
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelModels final : 
    public PanelEmptyWithDescr,
    public PanelBase<PanelCommonData>
{
public:
    PanelModels();
    ~PanelModels() override;

    void init() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
