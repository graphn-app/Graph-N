
#include "ui/panels/panel_text_editor.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"

namespace ui
{

PanelTextEditor::PanelTextEditor()
    : PanelBase(DlgObj::PanelTextEditor, ICON_LC_TEXT_INITIAL " Editor", ICON_LC_TEXT_INITIAL, DlgObjID[cast::to_szt(DlgObj::PanelTextEditor)])
{}

PanelTextEditor::~PanelTextEditor()
{
    LogF;
}

void PanelTextEditor::init()
{
    LogF;
    PanelBase::init();

}

void PanelTextEditor::free()
{
    LogF;

    //m_wf = 0;
    //m_node = nullptr;
    //m_pin = nullptr;
    //m_buffer = nullptr;
}

void PanelTextEditor::stop_all_threads()
{
    LogF;
}

//void PanelTextEditor::update(wid wf, data::NodeInstance* node, data::PinInstance* pin)
//{
//    if (pin->m_state->type != data::PinType::STRING || !pin->get_object()->multiline)
//    {
//        LogInternalLogic("Pin " << pin->get_object()->name << " is not multiline string");
//        return;
//    }
//
//    m_wf = wf;
//    m_node = node;
//    m_pin = pin;
//    m_buffer = data::Pin::get_string(pin->m_state->value);
//}

DlgCode PanelTextEditor::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
    }
    return end_panel(opened);
}

} // namespace ui
