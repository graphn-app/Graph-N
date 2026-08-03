
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelNotes final : 
    public PanelEmptyWithDescr,
    public PanelBase<PanelCommonData>
{
public:
    PanelNotes();
    ~PanelNotes() noexcept override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
