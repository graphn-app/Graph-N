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
    case js::REROUTE:
    case js::REROUTE_PYSSSSS:
    case js::REROUTE_RGTHREE:
        node = std::make_unique<data::Node>(js::REROUTE_CLASS);
        node->name = node->display_name = "Reroute";
        node->category = "utils";
        node->description = "Reroute Node Purpose: Mainly used to organize the logic of overly long connection lines in the " APP_COMFYUI " workflow.";
        node->python_module = VANILLA;
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanRename);
        node->flags.add(NodeObjFlags::SpecReroute);
        break;

    case js::PRIMITIVE_NODE:
        node = std::make_unique<data::Node>(js::PRIMITIVE_NODE_CLASS);
        node->name = node->display_name = "Primitive";
        node->category = "utils";
        node->description = "Primitive values like numbers, strings, and booleans.";
        node->python_module = VANILLA;
        node->flags.remove(NodeObjFlags::CanBypass);
        node->flags.add(NodeObjFlags::SpecPrimitive);
        break;

    case js::NOTE:
        node = std::make_unique<data::Node>(js::NOTE_CLASS);
        node->name = node->display_name = "Note";
        node->category = "utils";
        node->description = "Node to add annotations to a workflow.";
        node->python_module = VANILLA;
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecNote);
        break;

    case js::MARKDOWN_NOTE:
        node = std::make_unique<data::Node>(js::MARKDOWN_NOTE_CLASS);
        node->name = node->display_name = "Markdown Note";
        node->category = "utils";
        node->description = "Node to add annotations to a workflow. It supports text formatting using Markdown syntax.";
        node->python_module = VANILLA;
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecMdNote);
        break;

    case js::FAST_BYPASSER_RGTHREE:
        node = std::make_unique<data::Node>(js::FAST_BYPASSER_RGTHREE_CLASS);
        node->name = node->display_name = "Fast Bypasser (rgthree)";
        node->category = "rgthree";
        node->description = "A panel to quickly toggle directly connected nodes to Bypass mode";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastBypasserRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::NoHistoryOnPinValChange, NodeObjFlags::NoPinCagMenu);
        break;

    case js::FAST_MUTER_RGTHREE:
        node = std::make_unique<data::Node>(js::FAST_MUTER_RGTHREE_CLASS);
        node->name = node->display_name = "Fast Muter (rgthree)";
        node->category = "rgthree";
        node->description = "A panel to quickly toggle directly connected nodes to Mute mode";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastMuterRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::NoHistoryOnPinValChange, NodeObjFlags::NoPinCagMenu);
        break;

    case js::FAST_GROUPS_BYPASSER_RGTHREE:
        node = std::make_unique<data::Node>(js::FAST_GROUPS_BYPASSER_RGTHREE_CLASS);
        node->name = node->display_name = "Fast Groups Bypasser (rgthree)";
        node->category = "rgthree";
        node->description = "Automatically collects a list of groups on the canvas and allows bypassing all nodes within selected groups";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastGBypasserRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::NoHistoryOnPinValChange, NodeObjFlags::NoPinCagMenu);
        break;

    case js::FAST_GROUPS_MUTER_RGTHREE:
        node = std::make_unique<data::Node>(js::FAST_GROUPS_MUTER_RGTHREE_CLASS);
        node->name = node->display_name = "Fast Groups Muter (rgthree)";
        node->category = "rgthree";
        node->description = "Automatically collects a list of groups on the canvas and allows muting all nodes within selected groups";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastGMuterRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::NoHistoryOnPinValChange, NodeObjFlags::NoPinCagMenu);
        break;

    case js::FAST_ACTIONS_BUTTON_RGTHREE:
        node = std::make_unique<data::Node>(js::FAST_ACTIONS_BUTTON_RGTHREE_CLASS);
        node->name = node->display_name = "Fast Actions Button (rgthree)";
        node->category = "rgthree";
        node->description = "Executes assigned actions (Mute, Bypass, Enable) on connected nodes with a single click of an interface button";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecFastActRgthree, NodeObjFlags::SpecSwitherRgthree, NodeObjFlags::CustomJsNode, NodeObjFlags::NoPinCagMenu);
        break;

    case js::BOOKMARK_RGTHREE:
        node = std::make_unique<data::Node>(js::BOOKMARK_RGTHREE_CLASS);
        node->name = node->display_name = "Bookmark (rgthree)";
        node->category = "rgthree";
        node->description = "A navigational node that saves a specific canvas location and zoom level, allowing users to quickly jump (pan and zoom) to this area of the workflow using an assigned shortcut key";
        node->python_module = "frontend-only";
        node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
        node->flags.add(NodeObjFlags::SpecBookmarkRgthree, NodeObjFlags::CustomJsNode, NodeObjFlags::NoPinCagMenu);
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
