
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelMarkdownPreview final : 
    public PanelBase<PanelCommonData>
{
public:
    PanelMarkdownPreview();
    ~PanelMarkdownPreview() noexcept override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
