/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/alive.h"

namespace data
{
struct Node;
class NodeInstance;
class PinInstance;
}
namespace mngr
{
class NodesManager;
}

namespace ui
{

class CustomJsNode final : public IsAlive
{
public:
    CustomJsNode() noexcept = default;
    ~CustomJsNode() noexcept = default;

    void test(mngr::NodesManager* manager, data::Node* node);
    void init(data::NodeInstance* node);
    void draw(data::NodeInstance* node, float off, float width);
    void handle_pin_value_changed(const data::NodeInstance* node, data::PinInstance* pin);
    bool handle_context_menu();
    void handle_history(uint64_t id, wid wf_id, iid node_id, const blob::Blob& value);
    void handle_edited_image(wid wf_id, iid node_id, const std::string& image_name, const std::string& image_dir);

private:
    void draw_FastActionsButtonRgthree(data::NodeInstance* node, float off, float width);
    void draw_PowerLoraLoaderRgthree(data::NodeInstance* node, float off, float width);
    void draw_CustomCombo(data::NodeInstance* node, float off, float width);
    void draw_Load3D(data::NodeInstance* node, float off, float width);
    void draw_Primitive(data::NodeInstance* node, float off, float width);

    void handle_insert_lora_rgthree(const data::NodeInstance* node, data::PinInstance* pin);
    void handle_bookmark_rgthree(const data::NodeInstance* node, data::PinInstance* pin);
    bool handle_context_menu_lora_loader_rgthree();
    void handle_history_lora_loader_rgthree(wid wf_id, iid node_id, const blob::Blob& value);

    void handle_impact_preview(wid wf_id, iid node_id, const std::string& image_name, const std::string& image_dir);

private:
    struct ContextMenuData final
    {
        hid id = 0;
        wid wf_id = 0;
        data::NodeInstance* node = nullptr;
        data::PinInstance* pin = nullptr;
        const char* menu_id = nullptr;
        bool menu_inited = false;
        bool menu_open = false;
    };

    ContextMenuData m_js_context_menu_data;
};

} // namespace ui
