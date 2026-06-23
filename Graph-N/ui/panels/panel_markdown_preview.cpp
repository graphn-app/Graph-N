
#include "ui/panels/panel_markdown_preview.h"

namespace ui
{

PanelMarkdownPreview::PanelMarkdownPreview()
    : PanelBase(DlgObj::PanelMarkdownPreview, "Markdown Preview")
{}

PanelMarkdownPreview::~PanelMarkdownPreview()
{
    LogF;
}

void PanelMarkdownPreview::init()
{
    LogF;
    PanelBase::init();
}

void PanelMarkdownPreview::free()
{
    LogF;
}

void PanelMarkdownPreview::stop_all_threads()
{
    LogF;
}

DlgCode PanelMarkdownPreview::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
    }
    return end_panel(opened);
}

} // namespace ui
