/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelNotes final
    : public PanelEmptyWithDescr
    , public PanelBase<PanelCommonData>
{
public:
    PanelNotes();
    ~PanelNotes() noexcept override;

private:
    DlgCode show(bool& focused) override;

private:
};


} // namespace ui
