#include "ui/panels/panel_markdown_preview.h"

namespace ui
{

PanelMarkdownPreview::PanelMarkdownPreview()
    : PanelBase(DlgObj::PanelMarkdownPreview, "Markdown Preview")
{}

PanelMarkdownPreview::~PanelMarkdownPreview() noexcept = default;

DlgCode PanelMarkdownPreview::show(bool& focused) noexcept
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
    }
    return end_panel(opened);
}

} // namespace ui
