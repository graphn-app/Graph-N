/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;

class WfSerializerToJson final
{
public:
    WfSerializerToJson(const Workflow* wf) noexcept;
    ~WfSerializerToJson() noexcept = default;

    NO_COPY_MOVE(WfSerializerToJson);

    o_json serialize();

private:
    iid serialize_nodes();
    void serialize_pins(o_json& j);
    void serialize_widgets(o_json& j);
    iid serialize_links();
    void serialize_groups();

    void serialize_js_node(o_json& j);

    void serialize_cni_node(o_json& j);
    void serialize_cni_node_primitive(o_json& j);
    void serialize_cni_node_reroute(o_json& j);
    void serialize_cni_node_fast_switcher_rgthree(o_json& j);
    void serialize_cni_node_fast_group_switcher_rgthree(o_json& j);
    void serialize_cni_node_fast_action_rgthree(o_json& j);
    void serialize_cni_node_power_lora_loader_rgthree(o_json& j);
    void serialize_cni_node_impact_switch(o_json& j);
    void serialize_cni_node_impact_insversed_switch(o_json& j);

private:
    const Workflow* m_wf = nullptr;
    o_json m_json;
    const NodeInstance* m_curr_node = nullptr;
};

} // namespace data
