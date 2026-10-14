#ifdef _DEBUG
#include "ui/dialogs/debug_wf_view.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/pin.h"
#include "core/workflow/link.h"
#include "core/workflow/custom_nodes/cni_reroute.h"
#include "core/workflow/custom_nodes/cni_primitive.h"

namespace ui
{

int32_t DebugWfView::m_global_id = 0;

namespace
{

template<typename T>
struct FlagDef
{
    T flag;
    const char* name;
};

#define DEF_FLAG(e, f) { e::f, #f }

constexpr FlagDef<NodeInstFlags> g_node_inst_flags[] =
{
    DEF_FLAG(NodeInstFlags, Group), DEF_FLAG(NodeInstFlags, CanBeDeleted), DEF_FLAG(NodeInstFlags, CNI),
    DEF_FLAG(NodeInstFlags, RerouteSplitted), DEF_FLAG(NodeInstFlags, RerouteSplitted_R),
    DEF_FLAG(NodeInstFlags, Hidden), DEF_FLAG(NodeInstFlags, PosSet), DEF_FLAG(NodeInstFlags, SizeSet),
    DEF_FLAG(NodeInstFlags, IgnoreSizeOnFrameEnd), DEF_FLAG(NodeInstFlags, PinnedSet), DEF_FLAG(NodeInstFlags, ShowHidden),
    DEF_FLAG(NodeInstFlags, Deserialization), DEF_FLAG(NodeInstFlags, Pinned), DEF_FLAG(NodeInstFlags, Collapsed),
    DEF_FLAG(NodeInstFlags, ModeOnEvent), DEF_FLAG(NodeInstFlags, ModeMute), DEF_FLAG(NodeInstFlags, ModeOnTrigger),
    DEF_FLAG(NodeInstFlags, ModeBypass), DEF_FLAG(NodeInstFlags, HasInPins), DEF_FLAG(NodeInstFlags, HasOutPins),
    DEF_FLAG(NodeInstFlags, HasWidgets), DEF_FLAG(NodeInstFlags, JsonApiDontExport), DEF_FLAG(NodeInstFlags, JsonCustomSerialization),
    DEF_FLAG(NodeInstFlags, BlobCustomSerialization), DEF_FLAG(NodeInstFlags, MaskEditorReady), DEF_FLAG(NodeInstFlags, PosEditorReady),
    DEF_FLAG(NodeInstFlags, MaskEditorRunning), DEF_FLAG(NodeInstFlags, PosEditorRunning), DEF_FLAG(NodeInstFlags, JsonApiCustomSerialization),
    DEF_FLAG(NodeInstFlags, __AVAILABLE__1)
};

constexpr FlagDef<PinInstFlags> g_pin_inst_flags[] =
{
    DEF_FLAG(PinInstFlags, Hidden), DEF_FLAG(PinInstFlags, RerouteSplittingPin), DEF_FLAG(PinInstFlags, Multiline),
    DEF_FLAG(PinInstFlags, MultilineSet), DEF_FLAG(PinInstFlags, ColorHSV), DEF_FLAG(PinInstFlags, ColorHex),
    DEF_FLAG(PinInstFlags, ColorRaw), DEF_FLAG(PinInstFlags, ColorFloat)
};

constexpr FlagDef<NodeObjFlags> g_node_obj_flags[] =
{
    DEF_FLAG(NodeObjFlags, HasInput), DEF_FLAG(NodeObjFlags, HasOutput), DEF_FLAG(NodeObjFlags, OutputNode),
    DEF_FLAG(NodeObjFlags, SpecReroute), DEF_FLAG(NodeObjFlags, SpecPrimitive), DEF_FLAG(NodeObjFlags, SpecNote), DEF_FLAG(NodeObjFlags, SpecMdNote),
    DEF_FLAG(NodeObjFlags, SpecFastBypasserRgthree), DEF_FLAG(NodeObjFlags, SpecFastMuterRgthree), DEF_FLAG(NodeObjFlags, SpecFastGMuterRgthree),
    DEF_FLAG(NodeObjFlags, SpecFastGBypasserRgthree), DEF_FLAG(NodeObjFlags, SpecFastActRgthree), DEF_FLAG(NodeObjFlags, SpecBookmarkRgthree),
    DEF_FLAG(NodeObjFlags, SpecSwitherRgthree), 
    DEF_FLAG(NodeObjFlags, Dummy), DEF_FLAG(NodeObjFlags, CanBypass), DEF_FLAG(NodeObjFlags, CanMute),
    DEF_FLAG(NodeObjFlags, CanRename), DEF_FLAG(NodeObjFlags, Experimental), DEF_FLAG(NodeObjFlags, Deprecated),
    DEF_FLAG(NodeObjFlags, ApiNode), DEF_FLAG(NodeObjFlags, CustomMediaWidget), DEF_FLAG(NodeObjFlags, CustomMediaAudio),
    DEF_FLAG(NodeObjFlags, CustomMediaVideo), DEF_FLAG(NodeObjFlags, CustomMediaImage), DEF_FLAG(NodeObjFlags, CustomMediaText),
    DEF_FLAG(NodeObjFlags, RestrictCfg), DEF_FLAG(NodeObjFlags, CustomMediaUploadButton),
    DEF_FLAG(NodeObjFlags, CustomMediaFromSource), DEF_FLAG(NodeObjFlags, CustomMediaFromOutput), DEF_FLAG(NodeObjFlags, DevOnly),
    DEF_FLAG(NodeObjFlags, CustomJsNode)
};

constexpr FlagDef<PinObjFlags> g_pin_obj_flags[] =
{
    DEF_FLAG(PinObjFlags, Input), DEF_FLAG(PinObjFlags, Output),
    DEF_FLAG(PinObjFlags, Required), DEF_FLAG(PinObjFlags, Optional), DEF_FLAG(PinObjFlags, CanLink),
    DEF_FLAG(PinObjFlags, CanUnlink), DEF_FLAG(PinObjFlags, CanRename), DEF_FLAG(PinObjFlags, Advanced),
    DEF_FLAG(PinObjFlags, InputCanBeWidget), DEF_FLAG(PinObjFlags, ImageUpload), DEF_FLAG(PinObjFlags, AudioUpload),
    DEF_FLAG(PinObjFlags, VideoUpload), DEF_FLAG(PinObjFlags, FileUpload), DEF_FLAG(PinObjFlags, TypeAny),
    DEF_FLAG(PinObjFlags, Multiline), DEF_FLAG(PinObjFlags, ClrAlphaEnabled), DEF_FLAG(PinObjFlags, CagRequire),
    DEF_FLAG(PinObjFlags, DenyMultilineSwitch), DEF_FLAG(PinObjFlags, OutputIsList), DEF_FLAG(PinObjFlags, ForceInput),
    DEF_FLAG(PinObjFlags, DefInput), DEF_FLAG(PinObjFlags, Lazy), DEF_FLAG(PinObjFlags, Multiselect),
    DEF_FLAG(PinObjFlags, DisplAsNumber), DEF_FLAG(PinObjFlags, DisplAsColor), DEF_FLAG(PinObjFlags, DisplAsSlider),
    DEF_FLAG(PinObjFlags, DynamicPrompts)
};

template<typename T, size_t N>
void DrawFlagsEditor(const char* label, ObjectFlags<T>& flags, const FlagDef<T> (&defs)[N])
{
    if (ImGui::TreeNode(label))
    {
        for (size_t i = 0; i < N; ++i)
        {
            bool is_set = flags.has(defs[i].flag);
            if (ImGui::Checkbox(defs[i].name, &is_set))
            {
                if (is_set) flags.add(defs[i].flag);
                else flags.remove(defs[i].flag);
            }
            if ((i + 1) % 3 != 0 && i != N - 1) ImGui::SameLine(0, 15.0f);
        }
        ImGui::TreePop();
    }
}

} // namespace


std::string DebugWfView::primitive_to_string(const Primitive& p) const
{
    if (p.is_null()) return "null";
    if (p.is_int()) return std::to_string(p.get_int());
    if (p.is_float()) return std::to_string(p.get_float());
    if (p.is_number()) return std::to_string(p.get_number().underlying_type());
    if (p.is_bool()) return p.get_bool() ? "true" : "false";
    if (p.is_string()) return p.get_string();
    return "unknown";
}

void DebugWfView::draw_color_square(const char* label, ImU32 col) const
{
    const ImVec4 col_v4 = ImGui::ColorConvertU32ToFloat4(col);
    ImGui::ColorButton(label, col_v4, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(16, 16));
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
}

bool DebugWfView::passes_filter(const std::string& text) const
{
    if (m_filter[0] == '\0')
        return true;
    const std::string lower_text = tools::str::to_lower_ascii(text);
    const std::string lower_filter = tools::str::to_lower_ascii(m_filter);
    return lower_text.find(lower_filter) != std::string::npos;
}

void DebugWfView::draw_wf_data()
{
    if (!ImGui::CollapsingHeader("Workflow meta data", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    const mngr::WfData& wf_data = m_wf->get_data();

    ImGui::Text("WID: %u | UUID: %s | Name: %s", wf_data.id, wf_data.uuid.c_str(), wf_data.name.c_str());

    ImGui::Separator();

    ImGui::BeginDisabled();
    ImGui::Checkbox("Read only", (bool*)&wf_data.ro); ImGui::SameLine();
    ImGui::Checkbox("Virtual", (bool*)&wf_data.virt); ImGui::SameLine();
    ImGui::Checkbox("Temp", (bool*)&wf_data.temp);

    ImGui::Checkbox("Valid", (bool*)&wf_data.valid); ImGui::SameLine();
    ImGui::Checkbox("Critical failure", (bool*)&wf_data.critical_failure);

    ImGui::Checkbox("Draft", (bool*)&wf_data.draft); ImGui::SameLine();
    ImGui::Checkbox("Dirty", (bool*)&wf_data.dirty);

    ImGui::Checkbox("Activated", (bool*)&wf_data.activated); ImGui::SameLine();
    ImGui::Checkbox("Pending delete", (bool*)&wf_data.pending_delete);
    ImGui::EndDisabled();

    ImGui::Separator();

    const auto format_time = [](bool exists, const tools::dt& time_val) -> std::string
    {
        if (!exists || time_val.null()) return "None";
        return time_val.as_full_date_readable(true);
    };

    ImGui::Text("Saved exist: %d, t: %s", wf_data.saved_exist, format_time(wf_data.saved_exist, wf_data.saved_time).c_str());
    ImGui::Text("Draft exist: %d, t: %s", wf_data.draft_exist, format_time(wf_data.draft_exist, wf_data.draft_time).c_str());
    ImGui::Text("Ext time use: %d, t: %s", wf_data.use_ext_time, format_time(wf_data.use_ext_time, wf_data.ext_time).c_str());
}

void DebugWfView::draw_validation()
{
    if (!ImGui::CollapsingHeader("Validation & dangling objects", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    const iid last_id = m_wf->get_last_child_id();
    const auto& nodes = m_wf->get_nodes_all();
    const auto& links = m_wf->get_links_all();
    const auto& all_pins = m_wf->get_all_pins();
    const auto& all_cni_pins = data::CNI_BaseObj::get_all_pins();

    // Gather active IIDs and Object pointers from the Graph
    std::unordered_set<iid> active_iids;
    std::unordered_set<const data::Pin*> active_pin_objects;
    bool has_critical_error = false;
    iid max_active_iid = 0;

    const auto check_active_iid = [&](iid id, const char* context)
    {
        if (id == 0)
        {
            has_critical_error = true;
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Critical alert: IID is 0! (%s)", context);
            return;
        }
        if (!active_iids.insert(id).second)
        {
            has_critical_error = true;
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Duplicate active IID: %u (%s)", id, context);
        }
        if (id > max_active_iid) max_active_iid = id;
    };

    for (const auto& node : nodes)
    {
        check_active_iid(node->get_instance_id(), "Node");

        for (auto* p : node->get_inputs())
        {
            check_active_iid(p->get_instance_id(), "PinIn");
            active_pin_objects.insert(p->get_object());
        }
        for (auto* p : node->get_outputs())
        {
            check_active_iid(p->get_instance_id(), "PinOut");
            active_pin_objects.insert(p->get_object());
        }
    }

    for (const auto& link : links)
    {
        check_active_iid(link->get_instance_id(), "Link");
    }

    // Find dangling instances in all_pins
    std::vector<data::PinInstance*> dangling_pin_insts;
    for (const auto& pin : all_pins)
    {
        if (active_iids.find(pin->get_instance_id()) == active_iids.end())
            dangling_pin_insts.push_back(pin.get());
    }

    // Find dangling objects in all_cni_pins
    std::vector<data::Pin*> dangling_pin_objs;
    for (const auto& pin_obj : all_cni_pins)
    {
        if (active_pin_objects.find(pin_obj.get()) == active_pin_objects.end())
            dangling_pin_objs.push_back(pin_obj.get());
    }

    // --- Report ---
    ImGui::Text("Active nodes: %zu | Active links: %zu", nodes.size(), links.size());
    ImGui::Text("Base CNI pin objects loaded: %zu", all_cni_pins.size());
    ImGui::Text("Last child ID: %u", last_id);

    if (has_critical_error)
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "CRITICAL: Graph contains IID=0 or duplicates!");
    else
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "OK: all active IIDs are valid and unique.");

    if (max_active_iid > last_id && max_active_iid != 0)
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "CRITICAL: max_active_iid (%u) > last_child_id (%u)!", max_active_iid, last_id);

    // Dangling Reports
    if (!dangling_pin_insts.empty())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Found %zu dangling Pin instances (in all_pins, not on nodes):", dangling_pin_insts.size());
        for (auto* pin : dangling_pin_insts)
        {
            ImGui::BulletText("IID: %u | Parent IID: %u | Name: %s | Ed: %llu", 
                pin->get_instance_id(), pin->get_parent_node(), pin->state().name.c_str(), (unsigned long long)pin->ed().id());
        }
    }

    if (!dangling_pin_objs.empty())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Found %zu dangling pin objects (in all_cni_pins, unused by graph):", dangling_pin_objs.size());
        for (auto* obj : dangling_pin_objs)
        {
            ImGui::BulletText("Name: %s | Hash: 0x%016llX", obj->def_state.name.c_str(), (unsigned long long)obj->get_hash());
        }
    }
}

void DebugWfView::draw_pin_details(data::PinInstance* pin, bool is_input)
{
    const data::PinState& state = pin->state();
    const data::Pin* pin_obj = pin->get_object();

    ImGui::PushID(pin);
    if (ImGui::TreeNode(pin, "IID: %u | Pin: %s | Links: %d", pin->get_instance_id(), state.name.empty() ? "UNNAMED" : state.name.c_str(), state.links))
    {
        ImGui::Text("Ed ID: %llu", (unsigned long long)pin->ed().id());
        ImGui::Text("Type: %s (enum %d)", state.type_str.c_str(), (int)state.type);

        if (!is_input && pin_obj)
            ImGui::Text("Out index: %d", pin_obj->out_index);

        if (state.converted || pin->flags().has(PinInstFlags::Multiline))
        {
            ImGui::Text("Value: %s", primitive_to_string(state.value).c_str());
            ImGui::Text("Min: %s | Max: %s | Step: %s", 
                primitive_to_string(state.min).c_str(), 
                primitive_to_string(state.max).c_str(), 
                primitive_to_string(state.step).c_str());
        }

        draw_color_square("Pin color", state.pin_color);

        DrawFlagsEditor("Pin inst flags", const_cast<ObjectFlags<PinInstFlags>&>(pin->flags()), g_pin_inst_flags);

        if (pin_obj)
        {
            DrawFlagsEditor("Pin obj flags", const_cast<ObjectFlags<PinObjFlags>&>(pin_obj->flags), g_pin_obj_flags);

            if (state.type == PinType::COMBO && !pin_obj->combo_items_str.empty())
            {
                if (ImGui::TreeNode("Combo items"))
                {
                    for (size_t i = 0; i < pin_obj->combo_items_str.size(); ++i)
                    {
                        ImGui::BulletText("[%zu] %s", i, pin_obj->combo_items_str[i].c_str());
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void DebugWfView::draw_nodes_list()
{
    if (!ImGui::CollapsingHeader("Nodes", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    std::multimap<iid, data::NodeInstance*> nodes;
    std::ranges::for_each(m_wf->get_nodes_all(), [&nodes](const std::unique_ptr<data::NodeInstance>& node) { nodes.insert({ node->get_instance_id(), node.get() }); });

    for (const auto [id, node] :nodes)
    {
        const data::Node* node_obj = node->get_object();
        std::string class_name = node_obj ? node_obj->node_class : "GROUP";

        std::string search_target = std::to_string(node->get_instance_id()) + node->get_name() + class_name;
        if (!passes_filter(search_target))
            continue;

        ImGui::PushID(node);
        if (ImGui::TreeNode(node, "IID: %u | Node: %s | Class: %s", node->get_instance_id(), node->get_name().c_str(), class_name.c_str()))
        {
            ImGui::Text("Or. name: %s", node->get_name_original().c_str());
            ImGui::Text("Ed ID: %llu", (unsigned long long)node->ed().id());
            ImGui::Text("Rect: (%.1f, %.1f) - (%.1f, %.1f)", node->m_rc.Min.x, node->m_rc.Min.y, node->m_rc.Max.x, node->m_rc.Max.y);

            draw_color_square("Node", node->m_clr_node);
            draw_color_square("Header", node->m_clr_head);

            DrawFlagsEditor("Node inst flags", const_cast<ObjectFlags<NodeInstFlags>&>(node->flags()), g_node_inst_flags);
            if (node_obj)
                DrawFlagsEditor("Node obj flags", const_cast<ObjectFlags<NodeObjFlags>&>(node_obj->flags), g_node_obj_flags);

            if (node->has_object() && node->obj_flags().has(NodeObjFlags::SpecReroute))
            {
                if (ImGui::TreeNode("Reroute data"))
                {
                    const data::CNI_Reroute* reroute = cast::strict<data::CNI_Reroute>(node);
                    iid splitted_node = 0, splitted_in_pin = 0, splitted_out_pin = 0, splitted_link = 0;
                    reroute->get_splitters(splitted_node, splitted_in_pin, splitted_out_pin, splitted_link);

                    ImGui::Text("Splitted node IID: %u | Link IID: %u", splitted_node, splitted_link);
                    ImGui::Text("Splitted in.pin IID: %u | Out.pin IID: %u", splitted_in_pin, splitted_out_pin);
                    ImGui::TreePop();
                }
            }

            if (node->has_object() && node->obj_flags().has(NodeObjFlags::SpecReroute))
            {
                if (ImGui::TreeNode("Primitive data"))
                {
                    const data::CNI_Primitive* primitive = cast::strict<data::CNI_Primitive>(node);
                    ImGui::Text("Is linked: %s", primitive->is_linked() ? "YES" : "NO");
                    ImGui::Text("Linked outs count: %zu", primitive->get_linked_outs().size());
                    ImGui::TreePop();
                }
            }

            if (ImGui::TreeNode(node + 1, "Inputs (%zu)", node->get_inputs().size()))
            {
                for (auto* pin : node->get_inputs())
                {
                    draw_pin_details(pin, true);
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode(node + 2, "Outputs (%zu)", node->get_outputs().size()))
            {
                for (auto* pin : node->get_outputs())
                {
                    draw_pin_details(pin, false);
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void DebugWfView::draw_links_list()
{
    if (!ImGui::CollapsingHeader("Links", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (const auto& link : m_wf->get_links_all())
    {
        const data::PinInstance* input = link->get_input();
        const data::PinInstance* output = link->get_output();

        const std::string in_name = input ? input->state().name : "NULL";
        const std::string out_name = output ? output->state().name : "NULL";

        if (!passes_filter(std::to_string(link->get_instance_id()) + in_name + out_name))
            continue;

        ImGui::PushID(link.get());
        if (ImGui::TreeNode(link.get(), "IID: %u | %s -> %s", link->get_instance_id(), out_name.c_str(), in_name.c_str()))
        {
            ImGui::Text("Ed ID: %llu", (unsigned long long)link->ed().id());

            const auto print_pin = [this](const data::PinInstance* p, const char* prefix)
            {
                const auto node = m_wf->get_node_by_instance(p->get_parent_node());
                if (p) ImGui::Text("%s: | %s, IID: %u | Node: %s, IID: %u", prefix, p->state().name.c_str(), p->get_instance_id(), node->get_name().c_str(), p->get_parent_node());
                else ImGui::TextColored(ImVec4(1,0,0,1), "%s: BROKEN (NULL)", prefix);
            };

            print_pin(output, "Src out pin");
            print_pin(input,  "Dst in  pin");

            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void DebugWfView::init(data::Workflow* wf) noexcept
{
    LogRAssert(wf, "Wf zero ptr", );

    m_wf = wf;
    m_id = ++m_global_id;
    m_name = fmt::format("Wf dbg: {}###WfDebug_Win_{}_{}", m_wf->get_data().name, m_wf->get_data().uuid, m_id);
}

DlgCode DebugWfView::show()
{
    LogRAssert(m_wf, "Wf zero ptr", DlgCode::Cancel);

    ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_FirstUseEver);

    bool is_open = true;
    if (!ImGui::Begin(m_name.c_str(), &is_open, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        return is_open ? DlgCode::Progress : DlgCode::Cancel;
    }

    ImGui::SetNextItemWidth(300.0f);
    ImGui::InputText("Filter (Name/ID/Class)", m_filter, sizeof(m_filter));
    ImGui::Spacing();
    ImGui::Separator();

    draw_wf_data();
    draw_validation();
    draw_nodes_list();
    draw_links_list();

    ImGui::End();

    return is_open ? DlgCode::Progress : DlgCode::Cancel;
}

} // namespace ui
#endif // #ifdef _DEBUG
