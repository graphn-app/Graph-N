
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelTextEditor final : 
    public PanelBase<PanelCommonData>
{
public:
    PanelTextEditor();
    ~PanelTextEditor() override;

    void init() override;
    void free() override;
    void stop_all_threads() override;

    //void update(wid wf, data::NodeInstance* node, data::PinInstance* pin);

private:
    DlgCode show(bool& focused) override;

private:
    //wid m_wf = 0;
    //data::NodeInstance* m_node = nullptr;
    //data::PinInstance* m_pin = nullptr;
    //std::string* m_buffer = nullptr;
};


} // namespace ui
