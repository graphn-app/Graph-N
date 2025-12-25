
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelNodes final : public PanelBase
{
public:
    PanelNodes();
    ~PanelNodes() override = default;

    void init() override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui