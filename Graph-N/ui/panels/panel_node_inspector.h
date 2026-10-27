/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/panels/panel_base.h"
#include "ui/node_drawer_ro.h"
#include "core/listeners.h"

namespace data
{
struct Node;
class NodeInstanceRo;
class PinInstance;
class Workflow;
}

namespace ui
{

struct PanelNodeInspectorData final : public PanelCommonData
{
    ImU32 clr_descr_frame = 0;			// description frame
    ImU32 clr_selected_node = 0;		// selected node text
};

class PanelNodeInspector final
    : public PanelEmptyWithDescr
    , public PanelBase<PanelNodeInspectorData>
    , public listener::ComfyUiObjectValidityListener
{
public:
    PanelNodeInspector();
    ~PanelNodeInspector() noexcept override;

    void init() override;

    void add_inspecting_node(data::Node* node);
    void remove_inspecting_node(data::Node* node);
    void reset_inspecting_nodes();
    bool is_node_inspecting(const data::Node* node) const noexcept;

    //ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() override;

private:
    DlgCode show(bool& focused) override;
    void show_nodes();
    void show_description(std::string* descr, const ImVec2& descr_sz, float max_w) noexcept;

private:
    data::Workflow* m_wf = nullptr;

    struct Data final
    {
        Data(data::Workflow* wf) : drawer(wf) {}
        data::NodeInstanceRo* node = nullptr;
        RoNodeDrawer drawer;
        float ideal_size = 0.0f;
        std::string* descr = nullptr;
        ImVec2 descr_sz;
    };
    std::vector<Data> m_inspecting;
};


} // namespace ui
