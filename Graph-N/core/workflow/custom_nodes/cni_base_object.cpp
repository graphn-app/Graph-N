/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{

std::list<std::unique_ptr<Pin>> CNI_BaseObj::m_pins;

void CNI_BaseObj::cni_recreate_base_object()
{
    m_base_obj = cni_make_base_object(get_object()->get_hash());
    if (m_base_obj)
        set_object(m_base_obj.get());
}

void CNI_BaseObj::init()
{
    cni_recreate_base_object();
    NodeInstanceBase::init();
    cni_init();
}

void CNI_BaseObj::init(const std::string& select)
{
    cni_recreate_base_object();
    NodeInstance::init(select);
    cni_init();
}

void CNI_BaseObj::init(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    cni_recreate_base_object();
    NodeInstance::init(copy_from_node, copy_pin_ids);
    cni_init();
}

std::unique_ptr<Node> CNI_BaseObj::cni_make_base_object(hid hash)
{
    std::unique_ptr<Node> node;

    switch (hash)
    {
    case js::VANILLA_REROUTE:
    case js::PYSSSSS_REROUTE:
    case js::RGTHREE_REROUTE:
        node = std::make_unique<data::Node>(js::VANILLA_REROUTE_CLASS);
        node->name = node->display_name = "Reroute";
        node->category = "utils";
        node->description = "Reroute Node Purpose: Mainly used to organize the logic of overly long connection lines in the " APP_COMFYUI " workflow.";
        node->python_module = VANILLA;
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanRename);
        node->flags.add(NodeObjFlags::SpecReroute);
        break;

    case js::VANILLA_PRIMITIVE_NODE:
    case js::RGTHREE_POWER_PRIMITIVE:
        node = std::make_unique<data::Node>(js::VANILLA_PRIMITIVE_NODE_CLASS);
        node->name = node->display_name = "Primitive";
        node->category = "utils";
        node->description = "Primitive values like numbers, strings, and booleans.";
        node->python_module = VANILLA;
        node->flags.remove(NodeObjFlags::CanBypass);
        node->flags.add(NodeObjFlags::SpecPrimitive);
        break;

    case js::VANILLA_NOTE:
        node = std::make_unique<data::Node>(js::VANILLA_NOTE_CLASS);
        node->name = node->display_name = "Note";
        node->category = "utils";
        node->description = "Node to add annotations to a workflow.";
        node->python_module = VANILLA;
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecNote);
        break;

    case js::VANILLA_MARKDOWN_NOTE:
        node = std::make_unique<data::Node>(js::VANILLA_MARKDOWN_NOTE_CLASS);
        node->name = node->display_name = "Markdown Note";
        node->category = "utils";
        node->description = "Node to add annotations to a workflow. It supports text formatting using Markdown syntax.";
        node->python_module = VANILLA;
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecMdNote);
        break;

    case js::RGTHREE_FAST_BYPASSER:
        node = std::make_unique<data::Node>(js::RGTHREE_FAST_BYPASSER_CLASS);
        node->name = node->display_name = "Fast Bypasser (rgthree)";
        node->category = "rgthree";
        node->description = "A panel to quickly toggle directly connected nodes to Bypass mode";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastBypasserRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::NoHistoryOnPinValChange, NodeObjFlags::NoPinCagMenu);
        break;

    case js::RGTHREE_FAST_MUTER:
        node = std::make_unique<data::Node>(js::RGTHREE_FAST_MUTER_CLASS);
        node->name = node->display_name = "Fast Muter (rgthree)";
        node->category = "rgthree";
        node->description = "A panel to quickly toggle directly connected nodes to Mute mode";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastMuterRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::NoHistoryOnPinValChange, NodeObjFlags::NoPinCagMenu);
        break;

    case js::RGTHREE_FAST_GROUPS_BYPASSER:
        node = std::make_unique<data::Node>(js::RGTHREE_FAST_GROUPS_BYPASSER_CLASS);
        node->name = node->display_name = "Fast Groups Bypasser (rgthree)";
        node->category = "rgthree";
        node->description = "Automatically collects a list of groups on the canvas and allows bypassing all nodes within selected groups";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastGBypasserRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::NoHistoryOnPinValChange, NodeObjFlags::NoPinCagMenu);
        break;

    case js::RGTHREE_FAST_GROUPS_MUTER:
        node = std::make_unique<data::Node>(js::RGTHREE_FAST_GROUPS_MUTER_CLASS);
        node->name = node->display_name = "Fast Groups Muter (rgthree)";
        node->category = "rgthree";
        node->description = "Automatically collects a list of groups on the canvas and allows muting all nodes within selected groups";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastGMuterRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::NoHistoryOnPinValChange, NodeObjFlags::NoPinCagMenu);
        break;

    case js::RGTHREE_FAST_ACTIONS_BUTTON:
        node = std::make_unique<data::Node>(js::RGTHREE_FAST_ACTIONS_BUTTON_CLASS);
        node->name = node->display_name = "Fast Actions Button (rgthree)";
        node->category = "rgthree";
        node->description = "Executes assigned actions (Mute, Bypass, Enable) on connected nodes with a single click of an interface button";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastActRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::CustomJsNode, NodeObjFlags::NoPinCagMenu);
        break;

    case js::RGTHREE_BOOKMARK:
        node = std::make_unique<data::Node>(js::RGTHREE_BOOKMARK_CLASS);
        node->name = node->display_name = "Bookmark (rgthree)";
        node->category = "rgthree";
        node->description = "A navigational node that saves a specific canvas location and zoom level, allowing users to quickly jump (pan and zoom) to this area of the workflow using an assigned shortcut key";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecBookmarkRgthree, NodeObjFlags::CustomJsNode, NodeObjFlags::NoPinCagMenu);
        break;

    case js::RGTHREE_POWER_LORA_LOADER:
        return {};

    case js::IMPACT_SWITCH:
    case js::IMPACT_LATENT_SWITCH:
    case js::IMPACT_SEGS_SWITCH:
        node = std::make_unique<data::Node>(js::IMPACT_SWITCH_CLASS);
        node->name = node->display_name = "Switch (Any)";
        node->category = "ImpactPack";
        node->description = "Selects and passes a single input from multiple available inputs to the output based on the 'select' index. Acts as a data multiplexer supporting any data type";
        node->python_module = "comfyui-impact-pack";
        break;

    case js::IMPACT_INVERSED_SWITCH:
        node = std::make_unique<data::Node>(js::IMPACT_INVERSED_SWITCH_CLASS);
        node->name = node->display_name = "Inversed Switch (Any)";
        node->category = "ImpactPack";
        node->description = "Routes a single input value to one of multiple available output slots based on the 'select' index. Unselected outputs remain inactive. Acts as a data demultiplexer supporting any data type";
        node->python_module = "comfyui-impact-pack";
        break;

    default:
        LogRErrorS("Unknown base object", nullptr);
    }

    node->flags.add(NodeObjFlags::RestrictCfg);
    node->python_module_hash = base::HashID::make_hash(node->python_module);
    node->finalize(nullptr);
    return node;
}

} // namespace data
