/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/serializer/wf_serializer_to_json.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"

namespace data
{

void WfSerializerToJson::serialize_cni_node(o_json& j)
{
    switch (m_curr_node->get_object()->get_hash())
    {
    case js::VANILLA_PRIMITIVE_NODE:
        return serialize_cni_node_primitive(j);
    case js::VANILLA_REROUTE:
        return serialize_cni_node_reroute(j);
    case js::RGTHREE_FAST_BYPASSER:
    case js::RGTHREE_FAST_MUTER:
        return serialize_cni_node_fast_switcher_rgthree(j);
    case js::RGTHREE_FAST_GROUPS_BYPASSER:
    case js::RGTHREE_FAST_GROUPS_MUTER:
        return serialize_cni_node_fast_group_switcher_rgthree(j);
    case js::RGTHREE_FAST_ACTIONS_BUTTON:
        return serialize_cni_node_fast_action_rgthree(j);
    case js::RGTHREE_BOOKMARK:
        return serialize_cni_node_bookmark_rgthree(j);
    case js::IMPACT_SWITCH:
        return serialize_cni_node_impact_switch(j);
    case js::IMPACT_INVERSED_SWITCH:
        return serialize_cni_node_impact_insversed_switch(j);
    }

    if (m_curr_node->obj_flags().has(NodeObjFlags::CustomJsNode))
        return serialize_js_node(j);

    LogError("Unhandled cni node");
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Primitive                      //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToJson::serialize_cni_node_primitive(o_json& j)
{
    const auto& outputs = m_curr_node->get_outputs();
    LogRAssert(outputs.size() == 1, "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", sz:" << outputs.size(), );

    j["properties"]["Run widget replace on values"] = false;
    j["inputs"] = o_json::array();

    auto jouts = o_json::array();
    {
        auto pin = outputs[0];
        o_json jout;

        jout["name"] = pin->state().name;
        jout["type"] = pin->state().type_str;
        jout["widget"]["name"] = pin->state().name;

        if (0 == pin->state().links)
            jout["links"] = nullptr;
        else
        {
            jout["links"] = o_json::array();
            for (auto& link : m_wf->get_links_all())
            {
                if (link->get_output()->get_instance_id() == pin->get_instance_id())
                {
                    jout["links"].push_back(link->get_instance_id());
                }
            }
        }

        jouts.push_back(std::move(jout));
    }
    j["outputs"] = std::move(jouts);

    serialize_widgets(j);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Reroute                        //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToJson::serialize_cni_node_reroute(o_json& j)
{
    const auto& inputs = m_curr_node->get_inputs();
    const auto& outputs = m_curr_node->get_outputs();
    LogRAssert(inputs.size() == outputs.size(), "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", sz1:" << inputs.size() << ", sz2:" << outputs.size(), );

    j["properties"]["showOutputText"] = false;
    j["properties"]["horizontal"] = false;

    size_t active_idx = inputs.size();
    for (size_t i = 0; i < inputs.size(); i++)
    {
        if (inputs[i]->flags().has_not(PinInstFlags::Hidden))
        {
            LogRAssert(active_idx == inputs.size(), "Reroute wasn't splitted, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id(), );
            active_idx = i;
        }
    }
    LogRAssert(active_idx != inputs.size(), "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id(), );
    LogRAssert(outputs[active_idx]->flags().has_not(PinInstFlags::Hidden), "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id(), );

    const PinInstance* in = inputs[active_idx];
    const PinInstance* out = outputs[active_idx];
    LogRAssert(in && out, "Invalid node definition. Zero ptrs, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id(), );

    const auto get_links = [this](iid pin_id)
    {
        std::vector<iid> links;
        for (auto& link : m_wf->get_links_all())
        {
            if (link->get_input()->get_instance_id() == pin_id || link->get_output()->get_instance_id() == pin_id)
            {
                links.push_back(link->get_instance_id());
            }
        }
        return links;
    };

    auto jins = o_json::array();
    if (in->state().links > 0)
    {
        o_json jin;
        jin["name"] = "";
        jin["type"] = "*";
        if (PinType::ANY == in->state().type)
        {
            jin["link"] = nullptr;
        }
        else
        {
            const auto links = get_links(in->get_instance_id());
            LogRAssert(links.size() == 1, "Input pin has more than 1 links. w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", p:" << in->get_instance_id() << ":" << in->state().type_str << ", sz:" << links.size(), );
            jin["link"] = links.front();
            jin["widget"]["name"] = "value";
        }
        jins.push_back(std::move(jin));
    }

    o_json jout;
    jout["name"] = "";
    if (in->state().links > 0)
        jout["type"] = out->state().type_str;
    else
        jout["type"] = "*";

    if (out->state().links > 0)
    {
        const auto links = get_links(out->get_instance_id());
        LogRAssert(!links.empty(), "Output pin has no links. w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", p:" << out->get_instance_id() << ":" << out->state().type_str, );
        jout["links"] = links;
    }
    else
    {
        jout["links"] = nullptr;
    }
    auto jouts = o_json::array();
    jouts.push_back(std::move(jout));

    j["inputs"] = std::move(jins);
    j["outputs"] = std::move(jouts);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FashModeSwitcherRgthree                       ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToJson::serialize_cni_node_fast_switcher_rgthree(o_json& j)
{
    const auto& inputs = m_curr_node->get_inputs();
    const auto& outputs = m_curr_node->get_outputs();
    LogRAssert(inputs.size() >= 1 && outputs.size() == 1, "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", sz:" << outputs.size(), );

    j["properties"]["toggleRestriction"] = "default";

    auto jins = o_json::array();
    {
        for (auto in : inputs)
        {
            if (0 == in->state().links)
                continue;

            o_json jin;
            jin["dir"] = 3;
            jin["name"] = in->state().name;
            jin["type"] = "*";

            for (auto& link : m_wf->get_links_all())
            {
                if (link->get_input()->get_instance_id() == in->get_instance_id())
                {
                    jin["link"] = link->get_instance_id();
                    break;
                }
            }

            jins.push_back(std::move(jin));
        }

        o_json jin_last;
        jin_last["dir"] = 3;
        jin_last["name"] = "";
        jin_last["type"] = "*";
        jin_last["link"] = nullptr;
        jins.push_back(std::move(jin_last));
    }
    j["inputs"] = std::move(jins);

    auto jouts = o_json::array();
    {
        auto pin = outputs[0];
        o_json jout;

        jout["dir"] = 4;
        jout["name"] = "OPT_CONNECTION";
        jout["type"] = "*";

        if (0 == pin->state().links)
            jout["links"] = nullptr;
        else
        {
            jout["links"] = o_json::array();
            for (auto& link : m_wf->get_links_all())
            {
                if (link->get_output()->get_instance_id() == pin->get_instance_id())
                {
                    jout["links"].push_back(link->get_instance_id());
                }
            }
        }

        jouts.push_back(std::move(jout));
    }
    j["outputs"] = std::move(jouts);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FashModeSwitcherRgthree                       ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToJson::serialize_cni_node_fast_group_switcher_rgthree(o_json& j)
{
    const auto& outputs = m_curr_node->get_outputs();
    LogRAssert(outputs.size() == 1, "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", sz:" << outputs.size(), );

    j["properties"]["matchColors"] = "";
    j["properties"]["matchTitle"] = "";
    j["properties"]["showNav"] = true;
    j["properties"]["showAllGraphs"] = true;
    j["properties"]["sort"] = "position";
    j["properties"]["customSortAlphabet"] = "";
    j["properties"]["toggleRestriction"] = "default";

    j["inputs"] = o_json::array();

    auto jouts = o_json::array();
    {
        auto pin = outputs[0];
        o_json jout;

        jout["name"] = "OPT_CONNECTION";
        jout["type"] = "*";

        if (0 == pin->state().links)
            jout["links"] = nullptr;
        else
        {
            jout["links"] = o_json::array();
            for (auto& link : m_wf->get_links_all())
            {
                if (link->get_output()->get_instance_id() == pin->get_instance_id())
                {
                    jout["links"].push_back(link->get_instance_id());
                }
            }
        }

        jouts.push_back(std::move(jout));
    }
    j["outputs"] = std::move(jouts);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FastActionRgthree                             ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToJson::serialize_cni_node_fast_action_rgthree(o_json& j)
{
    const auto& inputs = m_curr_node->get_inputs();
    const auto& outputs = m_curr_node->get_outputs();
    LogRAssert(inputs.size() >= 1 && outputs.size() == 1, "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", sz:" << outputs.size(), );

    j["properties"]["buttonText"] = "🎬 Action!";
    j["properties"]["shortcutModifier"] = "alt";
    j["properties"]["shortcutKey"] = "";

    auto jwidgets = o_json::array();
    auto jins = o_json::array();
    {
        jwidgets.push_back("");
        for (auto in : inputs)
        {
            if (0 == in->state().links)
                continue;

            o_json jin;
            jin["dir"] = 3;
            jin["name"] = in->state().name;
            jin["type"] = "*";

            for (auto& link : m_wf->get_links_all())
            {
                if (link->get_input()->get_instance_id() == in->get_instance_id())
                {
                    jin["link"] = link->get_instance_id();
                    break;
                }
            }

            jins.push_back(std::move(jin));
            jwidgets.push_back(CNI_FastActionRgthree::string_from_mode(in->state().value));
        }

        o_json jin_last;
        jin_last["dir"] = 3;
        jin_last["name"] = "";
        jin_last["type"] = "*";
        jin_last["link"] = nullptr;
        jins.push_back(std::move(jin_last));
    }
    j["inputs"] = std::move(jins);
    j["widgets_values"] = std::move(jwidgets);
    j["outputs"] = o_json::array();
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    BookmarkRgthree                               ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToJson::serialize_cni_node_bookmark_rgthree(o_json& /*j*/)
{
    // TODO: CNI_BookmarkRgthree
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_SwitchImpact                                       //////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToJson::serialize_cni_node_impact_switch(o_json& j)
{
    const auto& inputs = m_curr_node->get_inputs();
    const auto& outputs = m_curr_node->get_outputs();
    LogRAssert(inputs.size() >= 1 && outputs.size() >= 3, "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", sz:" << outputs.size(), );

    j["properties"]["cnr_id"] = "comfyui-impact-pack";
    j["properties"]["ver"] = "429d0159ad429e64d2b3916e6e7be9c22d025c3c";
    j["properties"]["Node name for S&R"] = "ImpactSwitch";

    auto jins = o_json::array();
    {
        for (auto in : inputs)
        {
            o_json jin;
            jin["name"] = in->state().name;
            jin["type"] = in->state().type_str;

            if (in->obj_flags().has(PinObjFlags::InputCanBeWidget))
            {
                if (in->state().type == PinType::BOOLEAN)
                    continue;
                jin["widget"]["name"] = "select";
            }

            if (0 == in->state().links)
                jin["link"] = nullptr;
            else
            {
                for (auto& link : m_wf->get_links_all())
                {
                    if (link->get_input()->get_instance_id() == in->get_instance_id())
                    {
                        jin["link"] = link->get_instance_id();
                        break;
                    }
                }
            }

            jins.push_back(std::move(jin));
        }
    }
    j["inputs"] = std::move(jins);

    auto jouts = o_json::array();
    {
        for (auto out : outputs)
        {
            o_json jout;
            jout["name"] = out->state().name;
            jout["type"] = out->state().type_str;

            jout["links"] = o_json::array();
            if (out->state().links > 0)
            {
                for (auto& link : m_wf->get_links_all())
                {
                    if (link->get_input()->get_instance_id() == out->get_instance_id())
                    {
                        jout["links"].push_back(link->get_instance_id());
                        break;
                    }
                }
            }

            jins.push_back(std::move(jout));
        }
    }
    j["outputs"] = std::move(jouts);

    auto jwidgets = o_json::array();
    jwidgets.push_back(0);
    jwidgets.push_back(false);
    j["widgets_values"] = std::move(jwidgets);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_ReversedSwitchImpact                               //////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToJson::serialize_cni_node_impact_insversed_switch(o_json& j)
{
    const auto& inputs = m_curr_node->get_inputs();
    const auto& outputs = m_curr_node->get_outputs();
    LogRAssert(inputs.size() >= 3 && outputs.size() >= 1, "Invalid node definition, w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", sz:" << outputs.size(), );

    j["properties"]["cnr_id"] = "comfyui-impact-pack";
    j["properties"]["ver"] = "429d0159ad429e64d2b3916e6e7be9c22d025c3c";
    j["properties"]["Node name for S&R"] = "ImpactInversedSwitch";

    auto jins = o_json::array();
    {
        for (auto in : inputs)
        {
            o_json jin;
            jin["name"] = in->state().name;
            jin["type"] = in->state().type_str;

            if (in->obj_flags().has(PinObjFlags::InputCanBeWidget))
            {
                if (in->state().type == PinType::BOOLEAN)
                    continue;
                jin["widget"]["name"] = "select";
            }

            if (0 == in->state().links)
                jin["link"] = nullptr;
            else
            {
                for (auto& link : m_wf->get_links_all())
                {
                    if (link->get_input()->get_instance_id() == in->get_instance_id())
                    {
                        jin["link"] = link->get_instance_id();
                        break;
                    }
                }
            }

            jins.push_back(std::move(jin));
        }
    }
    j["inputs"] = std::move(jins);

    auto jouts = o_json::array();
    {
        for (auto out : outputs)
        {
            o_json jout;
            jout["name"] = out->state().name;
            jout["type"] = out->state().type_str;

            jout["links"] = o_json::array();
            if (out->state().links > 0)
            {
                for (auto& link : m_wf->get_links_all())
                {
                    if (link->get_input()->get_instance_id() == out->get_instance_id())
                    {
                        jout["links"].push_back(link->get_instance_id());
                        break;
                    }
                }
            }

            jins.push_back(std::move(jout));
        }
    }
    j["outputs"] = std::move(jouts);

    auto jwidgets = o_json::array();
    jwidgets.push_back(0);
    jwidgets.push_back(false);
    j["widgets_values"] = std::move(jwidgets);
}

} // namespace data
