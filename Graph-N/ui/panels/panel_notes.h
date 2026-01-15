
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelNotes final : public PanelBase<PanelCommonData>
{
public:
    PanelNotes();
    ~PanelNotes() override;

    void init() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
