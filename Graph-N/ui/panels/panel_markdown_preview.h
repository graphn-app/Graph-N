
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelMarkdownPreview final : 
    public PanelBase<PanelCommonData>
{
public:
    PanelMarkdownPreview();
    ~PanelMarkdownPreview() override;

    void init() override;
    void free() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
