
#include "ui/panels/panel_notes.h"

namespace ui
{

PanelNotes::PanelNotes()
    : PanelBase(DlgObj::PanelNotes, "Notes")
{}

PanelNotes::~PanelNotes() noexcept = default;

DlgCode PanelNotes::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
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
