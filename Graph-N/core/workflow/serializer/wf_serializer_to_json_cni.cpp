#include "core/workflow/serializer/wf_serializer_to_json.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"

namespace data
{

void WfSerializerToJson::serialize_cni_node(o_json& j)
{
    if (m_curr_node->obj_flags().has(NodeObjFlags::CustomJsNode))
        return serialize_js_node(j);

    if (CNI_Primitive::cni_is_same(m_curr_node))
        return serialize_cni_node_primitive(j);
    if (CNI_Reroute::cni_is_same(m_curr_node))
        return serialize_cni_node_reroute(j);
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
    for (size_t i = 0; i < inputs.size() - 1; i++)
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

} // namespace data
