/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/managers/workflows_manager.h"
#include "core/managers/db_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/history_manager.h"
#include "core/workflow/link.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/custom_nodes/cni_reroute.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/serializer/wf_serializers.h"
#include "ui/custom_nodes/custom_media_widget.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

WfsManager::WfsManager() noexcept = default;
WfsManager::~WfsManager() noexcept = default;

namespace
{
template <typename T>
void clear_wf_list(const T& list)
{
    for (auto& wf : list)
    {
        wf->clear_wf();
    }
};
}

void WfsManager::stop_and_cleanup()
{
    copy_reset();

    for (auto& [wf_id, wf] : m_wfs)
    {
        if (wf->get_data().pending_delete)
            m_labels.remove_item_for_all(wf_id);
        else
            close(wf->get_id(), false);
        wf->clear_wf();
    }
    std::set<uint32_t> cleanup;
    for (auto& label : m_labels.get_label_list())
    {
        if (label.members.empty() && label.auto_label)
            cleanup.insert(label.id);
    }
    for (auto label_id : cleanup)
    {
        m_labels.remove_label(label_id);
    }
    g::pOptsManager->items_wf_labels_save(m_labels);

    clear_wf_list(m_wfs_ro);
    clear_wf_list(m_wfs_virt);

    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_timers(this);

    m_active = 0;
    m_wfs.clear();
    m_wfs_ro.clear();
}

bool WfsManager::init()
{
    if (!Manager::init())
        return false;

    auto wf_datas = g::pDbManager->wf_load_meta_and_purge();
    for (auto& data : wf_datas)
    {
        if (data.name.starts_with("Workflow "))
        {
            const auto num = cast::to_u32(std::strtoul(data.name.c_str() + strlen("Workflow "), nullptr, 10));
            m_wf_name_counter = std::max(m_wf_name_counter, num);
        }

        create(std::move(data), false);
    }

    m_labels = g::pOptsManager->items_wf_labels_load();
    for (auto& label : m_labels.get_label_list())
    {
        std::set<hid> cleanup;
        for (auto wf_id : label.members)
        {
            if (!m_wfs.contains(cast::to<wid>(wf_id)))
                cleanup.insert(wf_id);
        }
        for (auto wf_id : cleanup)
        {
            m_labels.remove_item(label.id, wf_id);
        }
    }

    g::pGraphN->subscribe_timers(this);    
    return true;
}

void WfsManager::cleanup_frame()
{
    const auto clear_if = [](data::Workflow* wf, wid id)
    {
        if (wf->get_id() == id)
        {
            wf->clear_wf();
            return true;
        }
        return false;
    };

    for (auto wf_id : m_pending_cleanups.erase_wfs)
    {
        if (std::erase_if(m_wfs_virt, [wf_id, clear_if](const std::shared_ptr<data::Workflow>& ptr) { return clear_if(ptr.get(), wf_id); }) == 0)
        {
            if (std::erase_if(m_wfs_ro, [wf_id, clear_if](const std::unique_ptr<data::Workflow>& ptr) { return clear_if(ptr.get(), wf_id); }) == 0)
            {
                const auto it = m_wfs.find(wf_id);
                if (it != m_wfs.end())
                {
                    it->second->clear_wf();
                    m_wfs.erase(it);
                    Log("Wf removed. w:" << wf_id);
                }
            }
            else
                Log("RO wf removed. w:" << wf_id);
        }
        else
            Log("Virtual wf removed. w:" << wf_id);
    }

    for (auto wf_id : m_pending_cleanups.close_wfs)
    {
        close(wf_id, true);
    }

    m_pending_cleanups = {};
}

const WfData& WfsManager::create(bool generate_name, bool dump_to_db, bool ro)
{
    WfData data;
    data.ro = ro;
    if (generate_name)
        data.name = "Workflow " + std::to_string(++m_wf_name_counter);
    auto wf = create(std::move(data), dump_to_db);
    return wf->get_data();
}

const WfData& WfsManager::create_copy(wid source)
{
    if (0 == source)
    {
        static WfData empty;
        LogError("Zero wf source");
        return empty;
    }

    auto src_wf = access(source);
    
    WfData data;
    data.name = src_wf->get_data().name + " (copy)";
    auto wf = create(std::move(data), true);

    copy_begin(src_wf, ImVec2(0, 0));
    const auto nodes = src_wf->get_nodes_no_reroutes_r();
    for (auto node : nodes)
    {
        if (node->flags().has_not(NodeInstFlags::RerouteSplitted_R))
            copy_add(node);
    }
    copy_end(wf, ImVec2(0, 0), false);
    copy_begin(nullptr, {});

    return wf->get_data();
}

std::shared_ptr<data::Workflow> WfsManager::create_copy_virtual(const data::Workflow* src_wf)
{
    LogRAssert(src_wf, "Zero ptr", std::shared_ptr<data::Workflow>{});

    WfData data;
    data.name = src_wf->get_data().name;
    data.virt = true;
    auto wf = std::make_shared<data::Workflow>(std::move(data));
    Log("Virtual wf created. w:" << wf->get_id() << ", source:" << src_wf->get_id() << ", n:" << wf->get_data().name << ", u:" << wf->get_data().uuid);
    m_wfs_virt.push_back(wf);
    wf->init();

    copy_begin(src_wf, ImVec2(0, 0));
    const auto nodes = src_wf->get_nodes_no_reroutes_r();
    for (auto node : nodes)
    {
        copy_add(node);
    }
    copy_end(wf.get(), ImVec2(0, 0), false);
    copy_begin(nullptr, {});

    return wf;
}

const WfData& WfsManager::create_temp(const blob::Blob& source, const std::string& name, const std::string& label)
{
    if (source->empty())
    {
        static WfData empty;
        LogError("Empty blob");
        return empty;
    }

    WfData data;
    data.temp = true;
    data.name = name;
    auto wf = create(std::move(data), false);

    blob::Blob drawer_state;
    data::WfDeserializerFromBlob(wf, source).deserialize(drawer_state);
    data.draft = true;
    data.draft_exist = false;
    data.saved_exist = false;

    if (!label.empty())
    {
        auto label_id = find_label(label);
        if (0 == label_id)
        {
            label_id = create_label("", true);
            rename_label(label_id, label);
        }
        set_wf_labels(wf->get_id(), { label_id });
    }

    return wf->get_data();
}

bool WfsManager::is_loaded(wid wf_id) const
{
    LogRAssert(wf_id != 0, "Zero wf id", false);
    const auto* wf = access(wf_id);
    return !wf->get_nodes_all().empty();    
}

bool WfsManager::load(wid wf_id, blob::Blob& drawer_state)
{
    LogRAssert(wf_id != 0, "Zero wf id", false);
    auto wf = access(wf_id);
    auto& data = wf->get_data();
    if (!data.draft_exist && !data.saved_exist || data.ro)
        return false;

    blob::Blob blob;
    bool load_saved = !data.draft_exist;
    g::pDbManager->wf_load_blob(data.uuid, blob, load_saved);
    if (!blob->empty())
    {
        const bool result = data::WfDeserializerFromBlob(wf, blob).deserialize(drawer_state);
        data.draft = !load_saved || !result;
        return true;
    }
    return false;
}

data::Workflow* WfsManager::create(WfData&& data, bool dump_to_db)
{
    const bool ro = data.ro;
    auto wf = std::make_unique<data::Workflow>(std::move(data));
    Log("Wf created. w:" << wf->get_id() << ", ro:" << ro << ", n:" << wf->get_data().name << ", u:" << wf->get_data().uuid);
    auto wf_ptr = wf.get();
    wf_ptr->init();
    const auto& new_data = wf_ptr->get_data();
    if (ro)
        m_wfs_ro.push_back(std::move(wf));
    else
        m_wfs[new_data.id] = std::move(wf);
    if (dump_to_db)
        g::pDbManager->wf_update_meta(new_data);
    return wf_ptr;
}

data::Workflow* WfsManager::access(wid wf_id, bool allow_zero_ret /*= false*/) const
{
    LogRAssert(wf_id != 0, "Zero wf id", nullptr);

    const auto search_rw = [this, wf_id]() -> data::Workflow*
    {
        const auto it = m_wfs.find(wf_id);
        if (it != m_wfs.end())
            return it->second.get();
        return nullptr;
    };

    const auto search_virt = [this, wf_id]() -> data::Workflow*
    {
        const auto it = std::ranges::find_if(m_wfs_virt, [wf_id](const std::shared_ptr<data::Workflow>& ptr) noexcept { return ptr->get_id() == wf_id; });
        if (it != m_wfs_virt.end())
            return (*it).get();
        return nullptr;
    };

    const auto search_ro = [this, wf_id]() -> data::Workflow*
    {
        const auto it = std::ranges::find_if(m_wfs_ro, [wf_id](const std::unique_ptr<data::Workflow>& ptr) noexcept { return ptr->get_id() == wf_id; });
        if (it != m_wfs_ro.end())
            return (*it).get();
        return nullptr;
    };

    auto wf = search_rw();
    if (nullptr == wf) [[unlikely]]
        wf = search_virt();
    if (nullptr == wf) [[unlikely]]
        wf = search_ro();

    if (!allow_zero_ret)
        LogRAssert(wf, "No access. Wf not found. w:" << wf_id, nullptr);
    return wf;
}

void WfsManager::set_active(wid wf_id)
{
    if (m_active == wf_id)
        return;
    if (m_active != 0)
        dump(m_active);

    if (0 == wf_id)
    {
        m_active = 0;
    }
    else
    {
        const auto& data = access(wf_id)->get_data();
        m_active = data.id;
    }    
    Log("Wf activated. w:" << wf_id);
}

void WfsManager::update_name(wid wf_id, const std::string& name)
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    LogRAssert(!name.empty(), "Empty name", );
    
    auto& data = get_data(wf_id);
    data.name = name;
    g::pDbManager->wf_update_meta(data);
    Log("Wf renamed. w:" << wf_id << ", n:" << name);
}

namespace
{
std::vector<WfData*> filter_wfs(const std::unordered_map<wid, std::unique_ptr<data::Workflow>>& wfs, const std::function<bool(const WfData&)>& filter)
{
    std::vector<WfData*> ret;
    for (auto& [id, wf] : wfs)
    {
        auto& data = wf->get_data();
        if (filter(data))
            ret.push_back(&data);
    }
    return ret;
}
} // namespace

std::vector<WfData*> WfsManager::get_wfs() const
{
    return filter_wfs(m_wfs, [](const WfData&) { return true; });
}

std::vector<WfData*> WfsManager::get_undeleted() const
{
    return filter_wfs(m_wfs, [](const WfData& data) { return !data.pending_delete; });
}

std::vector<WfData*> WfsManager::get_deleted() const
{
    return filter_wfs(m_wfs, [](const WfData& data) { return data.pending_delete && !data.temp; });
}

void WfsManager::delete_wf(wid wf_id)
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    auto& data = get_data(wf_id);
    data.pending_delete = true;
    if (!data.temp)
        g::pDbManager->wf_update_meta(data);

    set_wf_labels(wf_id, {});
    Log("Wf deleted. w:" << wf_id);
}

void WfsManager::undelete_wf(wid wf_id)
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    auto& data = get_data(wf_id);
    data.pending_delete = false;
    g::pDbManager->wf_update_meta(data);
    Log("Wf restored. w:" << wf_id);
}

bool WfsManager::is_deleted(wid wf_id) const
{
    LogRAssert(wf_id != 0, "Zero wf id", false);
    auto& data = get_data(wf_id);
    return data.pending_delete;
}

void WfsManager::erase_wf(wid wf_id)
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    m_pending_cleanups.erase_wfs.push_back(wf_id);
}

uint32_t WfsManager::create_label(const std::string& name /*= ""*/, bool auto_del /*= false*/)
{
    const auto id = m_labels.add_label(auto_del, name);
    Log("New wf label added: " << std::as_const(m_labels).get_label(id)->name << " = " << id);
    g::pOptsManager->items_wf_labels_save(m_labels);
    return id;
}

bool WfsManager::rename_label(uint32_t label_id, const std::string& new_name)
{
    if (!m_labels.rename_label(label_id, new_name))
        return false;
    g::pOptsManager->items_wf_labels_save(m_labels);
    return true;
}

void WfsManager::remove_label(uint32_t label_id)
{
    m_labels.remove_label(label_id);
    g::pOptsManager->items_wf_labels_save(m_labels);
}

void WfsManager::set_wf_labels(wid wf_id, std::set<uint32_t> labels)
{
    auto& all_labels = m_labels.get_label_list();
    for (auto it = all_labels.begin(); it != all_labels.end(); ++it)
    {
        m_labels.remove_item(it->id, wf_id);
    }
    for (auto label_id : labels)
    {
        m_labels.add_item(label_id, wf_id);
    }
    g::pOptsManager->items_wf_labels_save(m_labels);
}

bool WfsManager::is_auto_label(uint32_t label_id) const
{
    auto label = std::as_const(m_labels).get_label(label_id);
    LogRAssert(label, "Label " << label_id << " not found", false);
    return label->auto_label;
}

std::list<wid> WfsManager::get_wfs_in_label(uint32_t label_id) const
{
    auto label = std::as_const(m_labels).get_label(label_id);
    LogRAssert(label, "Label " << label_id << " not found", {});

    std::list<wid> wfs;
    std::ranges::for_each(label->members, [&wfs](hid wf) { wfs.push_back(cast::to<wid>(wf)); });
    return wfs;
}

bool WfsManager::get_is_wf_in_label(wid wf_id, uint32_t label_id) const
{
    auto label = std::as_const(m_labels).get_label(label_id);
    LogRAssert(label, "Label " << label_id << " not found", {});
    return label->members.contains(wf_id);
}

void WfsManager::sort_wfs(std::list<wid>& wfs)
{
    wfs.sort([this](wid l_id, wid r_id)
    {
        auto l = access(l_id);
        auto r = access(r_id);
        return tools::str::to_lower_ascii(l->get_data().name) < tools::str::to_lower_ascii(r->get_data().name);
    });
}

void WfsManager::copy_begin(const data::Workflow* wf_src, std::optional<ImVec2> src_point)
{
    if (nullptr == wf_src)
    {
        Log("Copy reset");
        m_copy_src = nullptr;
        m_copied_nodes.clear();
        return;
    }
    m_copy_src = wf_src;
    m_copy_point = src_point;
    m_copied_nodes.clear();

    if (m_copy_point.has_value())
    {
        Log("Initiated copy from " << wf_src->get_id() << " at " << m_copy_point.value().x << "/" << m_copy_point.value().y);
    }
    else
    {
        Log("Initiated copy from " << wf_src->get_id() << " at undefined point");
    }
}

void WfsManager::copy_add(data::NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", );
    LogRAssert(m_copy_src, "Copy wasn't initiated", );
    LogRAssert(m_copy_src->get_id() == node->get_workflow_id(),
        "Node to copy is from diff wf. cpw:" << m_copy_src->get_id() << ", nw:" << node->get_workflow_id() << ", n:" << node->get_instance_id(), );

    if (node->flags().has(NodeInstFlags::RerouteSplitted))
        m_copied_nodes.insert(cast::strict<data::CNI_Reroute>(node)->get_splitted_base());
    else
        m_copied_nodes.insert(node);
    Log("Node added to copy. w:" << node->get_workflow_id() << ", n:" << node->get_instance_id());
}

void WfsManager::copy_del(data::NodeInstance* node)
{
    if (nullptr == m_copy_src) // valid case
        return;

    LogRAssert(node, "Zero ptr", );
    if (m_copied_nodes.erase(node) > 0)
    {
        Log("Node removed from copy. w:" << node->get_workflow_id() << ", n:" << node->get_instance_id());
    }
}

void WfsManager::copy_reset() noexcept
{
    m_copy_src = nullptr;
    m_copied_nodes.clear();
    m_copy_point = {};
}

void WfsManager::copy_end(data::Workflow* wf_dest, ImVec2 dest_point, bool center_dest_point)
{
    LogRAssert(wf_dest, "Zero ptr", );
    if (m_copied_nodes.empty() || nullptr == m_copy_src)
    {
        Log("No nodes to copy");
        return;
    }

    Log("Finishing copy. wsrc:" << m_copy_src->get_id() << ", wdst:" << wf_dest->get_id());

    ImVec2 src_pt(FLT_MAX, FLT_MAX);
    if (!m_copy_point.has_value())
    {
        // if the source point isn't provided, take the upper-left point of all copied nodes
        for (auto node : m_copied_nodes)
        {
            src_pt.x = std::min(src_pt.x, node->m_rc.Min.x);
            src_pt.y = std::min(src_pt.y, node->m_rc.Min.y);
        }
        Log("Copy source point autoset to " << src_pt.x << "/" << src_pt.y);
    }
    else
    {
        src_pt = m_copy_point.value();
        Log("Copy source point set to " << src_pt.x << "/" << src_pt.y);
    }

    // we might be asked to center the destination point in case of a paste via hotkey when the mouse pointer is out of canvas bounds
    // in this case, we just place the copied nodes at the center of the canvas
    if (center_dest_point)
    {
        ImVec2 src_max(-FLT_MAX, -FLT_MAX);
        for (auto node : m_copied_nodes)
        {
            const float node_right = node->m_rc.Min.x + node->m_rc.Max.x;
            const float node_bottom = node->m_rc.Min.y + node->m_rc.Max.y;

            src_max.x = std::max(src_max.x, node_right);
            src_max.y = std::max(src_max.y, node_bottom);
        }

        if (src_max.x != -FLT_MAX)
        {
            dest_point.x -= (src_max.x - src_pt.x) * 0.5f;
            dest_point.y -= (src_max.y - src_pt.y) * 0.5f;
        }
    }

    const auto& dest_wf_data = wf_dest->get_data();

    // a virtual copy of the workflow is created when a new task starts or when the workflow is being exported
    if (!dest_wf_data.virt)
        g::pHistoryManager->multi_action_begin(wf_dest->get_id(), "Paste");

    std::map<iid, std::pair<data::NodeInstance*, data::NodeInstance*>> curr_node_to_new_node; // src node iid -> src_node/dest_node
    wf_dest->set_last_child_id(m_copy_src->get_last_child_id());

    std::set<data::NodeInstance*> new_nodes;

    for (auto node : m_copied_nodes)
    {
        LogCAssert(node->flags().has_not(NodeInstFlags::RerouteSplitted_R), "Splitted node shouldn't be copied explicitly");

        ImRect dest_rect;
        dest_rect.Min.x = node->m_rc.Min.x - src_pt.x;
        dest_rect.Min.x = dest_point.x + dest_rect.Min.x;
        dest_rect.Min.y = node->m_rc.Min.y - src_pt.y;
        dest_rect.Min.y = dest_point.y + dest_rect.Min.y;
        dest_rect.Max = node->m_rc.Max;

        if (node->flags().has(NodeInstFlags::Group))
        {
            wf_dest->add_group(dest_rect, node->get_name());
            continue;
        }

        auto new_node = wf_dest->add_node(node, dest_wf_data.virt);
        new_nodes.insert(new_node);
        new_node->flags().add(NodeInstFlags::Deserialization);
        new_node->m_rc = dest_rect;

        if (node->flags().has(NodeInstFlags::RerouteSplitted))
            cast::strict<data::CNI_Reroute>(new_node)->adjust_r_pose();

        if (!dest_wf_data.virt)
            g::pHistoryManager->push_node_action(new_node, HistoryActionType::NodeAdd);
        else if (new_node->obj_flags().has_none(NodeObjFlags::SpecPrimitive, NodeObjFlags::SpecReroute, NodeObjFlags::SpecNote, NodeObjFlags::SpecMdNote))
        {
            // for a virtual copy, we need to mirror all instance ids
            std::ranges::for_each(node->get_inputs(), [new_node](data::PinInstance* pin)
            {
                const auto it = std::ranges::find_if(new_node->get_inputs(), [pin](data::PinInstance* new_pin) noexcept { return pin->get_object()->get_hash() == new_pin->get_object()->get_hash(); });
                if (it != new_node->get_inputs().end())
                    (*it)->set_instance_id(pin->get_instance_id());
                else
                    LogErrorS("Copied input pin not found. psrc:" << pin->get_instance_id());
                
            });
            std::ranges::for_each(node->get_outputs(), [new_node](data::PinInstance* pin)
            {
                const auto it = std::ranges::find_if(new_node->get_outputs(), [pin](data::PinInstance* new_pin) noexcept { return pin->get_object()->get_hash() == new_pin->get_object()->get_hash(); });
                if (it != new_node->get_outputs().end())
                    (*it)->set_instance_id(pin->get_instance_id());
                else
                    LogErrorS("Copied input pin not found. psrc:" << pin->get_instance_id());
            });
        }

        curr_node_to_new_node[node->get_instance_id()] = std::make_pair(node, new_node);
    }

    // mirror connections

    const auto links = m_copy_src->get_links_no_reroutes();
    for (auto link : links)
    {
        const auto* src_in_pin = link->get_input();
        const auto* src_out_pin = link->get_output();
        
        const auto in_it = curr_node_to_new_node.find(src_in_pin->get_parent_node());
        const auto out_it = curr_node_to_new_node.find(src_out_pin->get_parent_node());

        // valid case: one of the linked nodes wasn't copied
        if (in_it == curr_node_to_new_node.end() || out_it == curr_node_to_new_node.end())
            continue;

        const data::NodeInstance* dest_in_node = in_it->second.second;
        const data::NodeInstance* dest_out_node = out_it->second.second;
        data::PinInstance* dest_in_pin = nullptr;
        data::PinInstance* dest_out_pin = nullptr;

        // search for the same pin on the copied node
        // in the case of a reroute, we don't search by hash because all hashes are unique, but pin names are unique within the node

        for (auto pin : dest_in_node->get_inputs())
        {
            if (dest_in_node->obj_flags().has(NodeObjFlags::SpecReroute))
            {
                if (pin->state().name == src_in_pin->state().name)
                {
                    dest_in_pin = pin;
                    break;
                }
            }
            else if (pin->get_object()->get_hash() == src_in_pin->get_object()->get_hash())
            {
                dest_in_pin = pin;
                break;
            }
        }
        for (auto pin : dest_out_node->get_outputs())
        {
            if (dest_out_node->obj_flags().has(NodeObjFlags::SpecReroute))
            {
                if (pin->state().name == src_out_pin->state().name)
                {
                    dest_out_pin = pin;
                    break;
                }
            }
            else if (pin->get_object()->get_hash() == src_out_pin->get_object()->get_hash())
            {
                dest_out_pin = pin;
                break;
            }
        }
        LogRAssert(dest_in_pin && dest_out_pin, "Dest pin(s) not found. wsrc:" << m_copy_src->get_id() << ", wdst:" << wf_dest->get_id()
                << ", din:" << dest_in_node->get_instance_id() << ", sip:" << src_in_pin->get_instance_id() << ":" << (dest_in_pin != nullptr)
                << ", don:" << dest_out_node->get_instance_id() << ", sop:" << src_out_pin->get_instance_id() << ":" << (dest_out_pin != nullptr), );

        const auto* new_link = data::Link::link(dest_in_pin, dest_out_pin, dest_wf_data.virt ? link->get_instance_id() : 0);
        if (!dest_wf_data.virt)
            g::pHistoryManager->push_link_action(new_link, HistoryActionType::LinkAdd);
    }

    if (!dest_wf_data.virt)
        g::pHistoryManager->multi_action_end(wf_dest->get_id());

    const auto dest_nodes = wf_dest->get_nodes_no_reroutes_r();
    for (auto node : dest_nodes)
    {
        if (new_nodes.contains(node))
        {
            node->flags().remove(NodeInstFlags::Deserialization);
            node->on_deserialization_ends();
        }
    }

    if (!dest_wf_data.virt)
        g::pFlow->wf_reload_view(wf_dest->get_id());

    Log("Copied " << m_copied_nodes.size() << " nodes from " << m_copy_src->get_id() << " to " << wf_dest->get_id());
}

void WfsManager::close_and_purge()
{
    copy_reset();

    for (auto& [id, wf] : m_wfs)
    {
        auto& data = wf->get_data();

        if (!data.draft_exist && !data.saved_exist && wf->get_nodes_all().empty())
        {
            data.pending_delete = true;
            if (!data.temp)
                g::pDbManager->wf_update_meta(data);
            Log("Wf " << data.uuid << "###" << data.name << " (" << id << ") marked to pending delete");
        }
        else
            close(id, false);
    }
}

void WfsManager::close(wid wf_id, bool process_pending_deletion)
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    auto wf = access(wf_id);
    auto& data = wf->get_data();

    if (m_copy_src == wf)
        copy_reset();

    if (!process_pending_deletion)
    {
        g::pFlow->host_handle_wf_closure(wf_id);
        g::pFlow->custom_media_widgets().cleanup_outputs(wf_id);

        if (data.temp && data.dirty)
        {
            data.temp = false;
            g::pDbManager->wf_update_meta(data);
        }

        if (data.dirty && !data.temp)
            dump(wf_id);
        else if (data.temp)
            delete_wf(wf_id);

        m_pending_cleanups.close_wfs.push_back(wf_id);
        return;
    }

    wf->clear_wf();
    Log("Wf closed. w:" << wf_id);
}

void WfsManager::save(wid wf_id)
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    auto wf = access(wf_id);
    auto& data = wf->get_data();

    if (data.temp)
    {
        data.dirty = true;
        data.temp = false;
        g::pDbManager->wf_update_meta(data);
    }

    if (!data.dirty && !data.draft || data.ro)
        return;

    g::pHistoryManager->set_rollback_pos(wf_id);
    data.draft = false;
    data.saved_exist = true;
    data.draft_exist = false;
    data.dirty = true;
    data.temp = false;
    dump(wf_id);

    Log("Wf saved. w:" << wf_id);
}

void WfsManager::revert(wid wf_id, blob::Blob& drawer_state, bool& restore_drawer)
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    auto wf = access(wf_id);
    auto& data = wf->get_data();
    restore_drawer = false;

    const bool can_rollback = g::pHistoryManager->can_rollback(wf_id);
    if (!can_rollback && !data.saved_exist)
        return;

    if (can_rollback)
    {
        g::pHistoryManager->rollback(wf_id);
        Log("Wf " << wf_id << " reverted via history rollback");
    }
    else
    {
        blob::Blob blob;
        bool load_saved = true;
        g::pDbManager->wf_load_blob(data.uuid, blob, load_saved);
        LogAssert(load_saved, "Wf failed to load state. w:" << wf_id);

        const bool res = data::WfDeserializerFromBlob(wf, blob).deserialize(drawer_state);
        g::pHistoryManager->cleanup(wf_id);
        g::pHistoryManager->set_rollback_pos(wf->get_id());
        restore_drawer = true;
        Log("Wf " << wf_id << " reverted via hard reset, res: " << (res ? "ok" : "fail"));
    }

    wf->invalidate_current();
    data.dirty = false;
    data.draft = false;
    data.saved_exist = true;
}

void WfsManager::dump(wid wf_id) const
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    auto wf = access(wf_id);
    auto& data = wf->get_data();

    if (data.temp && data.dirty)
    {
        data.temp = false;
        g::pDbManager->wf_update_meta(data);
    }
    if (!data.dirty || data.critical_failure || data.ro)
        return;

    blob::Blob empty;
    const blob::Blob* drawer_state = nullptr;
    const auto* wf_action = wf->action();
    if (wf_action != nullptr)
        drawer_state = wf_action->get_drawer_state();
    if (nullptr == drawer_state)
        drawer_state = &empty;

    auto wr = data::WfSerializerToBlob(wf).serialize(*drawer_state);
    g::pDbManager->wf_update_blob(data.uuid, wr.release(), data.draft, data.use_ext_time ? data.ext_time : tools::dt());

    data.dirty = false;
    if (data.draft)
    {
        data.draft_exist = true;
        if (data.use_ext_time)
            data.draft_time = data.ext_time;
        else
            data.draft_time = tools::dt::from_current_date();
    }
    else
    {
        data.saved_exist = true;
        if (data.use_ext_time)
            data.saved_time = data.ext_time;
        else
            data.saved_time = tools::dt::from_current_date();
    }
    data.use_ext_time = false;
    
    Log("Wf dumped. w:" << wf_id << (data.draft ? ", draft" : ", explicit save"));
}

wid WfsManager::import_wf(const std::filesystem::path& from)
{
    Log("Import project from " << from);
    const auto ext = tools::str::to_lower_ascii(tools::path::path_to_string(from.extension()));
    const auto name = tools::path::path_to_string(from.stem());

    if (ext == ".json")
    {
        const auto data = tools::path::read_file(from, true);
        if (!data.empty())
            return import_wf_from_json(data, tools::path::path_to_string(from.stem()));
        return 0;
    }
    if (ext == ".png")
    {
        const auto data = tools::img::extract_workflow_from_png(from);
        if (!data.empty())
            return import_wf_from_json(data, tools::path::path_to_string(from.stem()));
        return 0;
    }
    if (ext == ".webp")
    {
        const auto data = tools::img::extract_workflow_from_webp(from);
        if (!data.empty())
            return import_wf_from_json(data, tools::path::path_to_string(from.stem()));
        return 0;
    }

    LogRError("Can't import, unsupported file type", 0);
}

wid WfsManager::import_wf_from_json(const std::string& jdata, const std::string& name, tools::dt use_time /*= {}*/, bool virt /*= false*/)
{
    try
    {
        Log("Start import json. f:" << name);
        const auto j = o_json::parse(jdata);

        WfData wf_data;
        wf_data.ro = false;
        wf_data.name = tools::path::path_to_string(std::filesystem::path(name).stem());
        if (!use_time.null())
        {
            wf_data.ext_time = use_time;
            wf_data.use_ext_time = true;
        }
        auto wf = create(std::move(wf_data), false);

        if (data::WfDeserializerFromJson(wf, j).deserialize() || data::WfDeserializerFromJsonApi(wf, j).deserialize())
        {
            if (!virt)
            {
                Log("Import project success, wf id set to " << wf->get_uuid() << " -> " << wf->get_id());
                g::pDbManager->wf_update_meta(wf->get_data());
                wf->get_data().dirty = true;
                wf->get_data().draft = true;
                save(wf->get_id());

                auto label = find_label(TEMP_LABEL_IMPORTED);
                if (0 == label)
                    label = create_label(TEMP_LABEL_IMPORTED, true);
                if (label != 0)
                    set_wf_labels(wf->get_id(), { label });
            }
            return wf->get_id();
        }

        Log("Import project failed");
        delete_wf(wf->get_id());
    }
    catch (const std::exception& ex)
    {
        LogExcep("Failed to parse json, err " << ex.what());
    }

    return 0;
}

void WfsManager::export_wf(wid wf_id, bool api, const std::filesystem::path& to)
{
    LogRAssert(wf_id != 0, "Zero wf id", );
    const auto wf_it = m_wfs.find(wf_id);
    LogRAssert(wf_it != m_wfs.end(), "Can't export - active wf not found. w:" << wf_id, );

    auto wf = wf_it->second.get();
    if (wf->get_nodes_all().empty())
    {
        blob::Blob dummy;
        load(wf->get_id(), dummy);
    }

    Log("Start export. w" << wf_id << ", API:" << api << ", f:" << to.string());
    o_json j;
    if (api)
        j = data::WfSerializerToJsonApi(wf).serialize();
    else
        j = data::WfSerializerToJson(wf).serialize();

    auto jdump = j.dump();
    if (!tools::path::make_file_path(to, true, jdump))
        Log("Export failed");
}

void WfsManager::on_1s_lazy_timer(int64_t /*total_seconds_elapsed*/)
{
    m_dump_counter++;
    if (m_dump_counter >= DRAFT_DUMP_INTERVAL)
    {
        if (ImGui::IsAnyMouseDown()) [[unlikely]]
        {
            Log("Canvas is busy, save later");
            return;
        }

        for (auto& wf : m_wfs)
        {
            dump(wf.first);
        }
        m_dump_counter = 0;
    }
}

} // namespace mngr
