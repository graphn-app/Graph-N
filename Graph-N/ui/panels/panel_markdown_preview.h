/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelMarkdownPreview final : public PanelBase<PanelCommonData>
{
public:
    PanelMarkdownPreview();
    ~PanelMarkdownPreview() noexcept override;

private:
    DlgCode show(bool& focused) noexcept override;

private:
};


} // namespace ui
