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
    NO_COPY_MOVE_CTORS(CustomJsNode);

    void test(mngr::NodesManager* manager, data::Node* node);
    void init(data::NodeInstance* node);
    void draw(data::NodeInstance* node, float off, float width);
    void handle_pin_value_changed(data::NodeInstance* node, data::PinInstance* pin);
    bool handle_context_menu();
    void handle_history(uint64_t id, wid wf_id, iid node_id, const blob::Blob& value);

private:
    void draw_FastActionsButtonRgthree(data::NodeInstance* node, float off, float width);
    void draw_CustomCombo(data::NodeInstance* node, float off, float width);
    void draw_Load3D(data::NodeInstance* node, float off, float width);

    void handle_insert_lora_rgthree(data::NodeInstance* node, data::PinInstance* pin);
};

} // namespace ui
