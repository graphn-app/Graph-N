
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/text_wrapper_ro.h"

namespace data
{
struct Node;
}

namespace ui
{

struct PanelNodeInspectorData final : public PanelCommonData
{
    ImU32 clr_descr_frame = 0;
};

class PanelNodeInspector final : public PanelBase<PanelNodeInspectorData>
{
public:
    PanelNodeInspector();
    ~PanelNodeInspector() override = default;

    void init() override;

    void set_inspecting_node(data::Node* node);

private:
    DlgCode show(bool& focused) override;

private:
    data::Node* m_inspecting_node = nullptr;
    TextWrapperRO m_descr_wrapper;
};


} // namespace ui