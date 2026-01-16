
#include "ui/panels/panel_notes.h"

namespace ui
{

PanelNotes::PanelNotes()
    : PanelBase(DlgObj::PanelNotes, ICON_LC_SCROLL_TEXT " Notes", ICON_LC_SCROLL_TEXT, "E25BB488-3298-4FB7-A5BD-9999C05AC2B8")
{}

PanelNotes::~PanelNotes()
{
    LogF;
}

void PanelNotes::init()
{
    LogF;
    PanelBase::init();

}

void PanelNotes::stop_all_threads()
{
    LogF;
}

DlgCode PanelNotes::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        show_empty_panel_descr(
            ICON_LC_INFO " Notes.",
            {},
                {
                    "Space for your ideas.",
                    "Use this window to write down prompts,",
                    "save generation parameters,",
                    "or list tasks for the current project.",
                    "Text is saved automatically.",
                    "",
                    "Add new note using Main Menu"
                }                
                );
    }
    return end_panel(opened);
}

} // namespace ui
