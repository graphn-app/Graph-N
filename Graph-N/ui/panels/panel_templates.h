
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelTemplates final
    : public PanelBase<PanelCommonData>
    , public PanelEmptyWithDescr
{
public:
    PanelTemplates();
    ~PanelTemplates() override;

    void init() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
