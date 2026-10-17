#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{
class PinInstance;

struct RoutedPinPair final
{
    std::string type = "*";
    iid in_id = 0;
    hid in_hash = 0;
    iid out_id = 0;
    hid out_hash = 0;
    std::string in_pin_name = "*";
    int32_t out_index = -1;
    bool hidden = false;
};
struct CNI_DeserializedReroute final : public CNI_DeserializedData
{
    std::vector<RoutedPinPair> pairs;
    std::map<PinType, int32_t> in_pin_types;
    int32_t next_out_index = 0;
    bool splitted = false;
    std::pair<ImRect, ImRect> poses;
    iid splitted_node = 0;
    iid splitter_in = 0;
    iid splitter_out = 0;
    iid splitter_link = 0;
};

struct LinkedIn final
{
    const PinInstance* external = nullptr;
    PinInstance* internal = nullptr;
};

struct LinkedOut final
{
    std::list<const PinInstance*> external;
    PinInstance* internal = nullptr;
};

struct LinkedPair final
{
    bool linked = false;
    bool ever_linked = false;
    bool restored = false;
    PinType type = PinType::_none;
    LinkedIn input;
    LinkedOut output;
};

struct RerouteData
{
    std::list<LinkedPair> m_pairs;
    int32_t m_next_out_index = 1;
};

class CNI_Reroute final : public CNI_BaseObj
{
public:
    CNI_Reroute(Node* obj, wid workflow_id, iid instance_id, iid instance_id2) noexcept;
    ~CNI_Reroute() noexcept;

    void cni_init() override;
    bool cni_is_applicable() const noexcept override { return true; }

    void add_pair(const RoutedPinPair& pair = {}, bool restored = false);
    std::pair<PinInstance*, PinInstance*> find_first_empty_pair_or_add_empty();

    bool cni_can_link(const PinInstance* p1, const PinInstance* p2) const noexcept override;
    void cni_pin_linked(PinInstance* p1, PinInstance* p2) override;
    void cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2) override;

    void cni_cleanup_base_object() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
    void cni_mirror_value_to(const PinInstance* src, std::vector<PinInstance*>& append_pins) const override;
    void on_deserialization_ends() override;

public:
    void split_to_parts();
    void merge_parts(bool to_left);

    NodeInstance* get_splitted_base() noexcept
    {
        if (flags().has(NodeInstFlags::RerouteSplitted_R)) return m_splitted_part;
        return this;
    }
    const NodeInstance* get_splitted_base() const noexcept
    {
        if (flags().has(NodeInstFlags::RerouteSplitted_R)) return m_splitted_part;
        return this;
    }
    NodeInstance* get_splitted_part() noexcept
    {
        if (flags().has(NodeInstFlags::RerouteSplitted_R)) return this;
        return m_splitted_part;
    }

    PinInstance* get_splitter_input() const noexcept
    {
        if (flags().has(NodeInstFlags::RerouteSplitted_R)) return m_splitted_pin;
        return nullptr;
    }
    PinInstance* get_splitter_output() const noexcept
    {
        if (flags().has(NodeInstFlags::RerouteSplitted_R)) return nullptr;
        return m_splitted_pin;
    }

    void get_splitters(iid& splitted_node, iid& in_pin, iid& out_pin, iid& link) const;

    std::pair<ImRect, ImRect> get_splitted_nodes_poses() const noexcept
    {
        if (flags().has(NodeInstFlags::RerouteSplitted_R)) return std::make_pair(m_splitted_part->m_rc, m_rc);
        return std::make_pair(m_rc, m_splitted_part->m_rc);
    }
    void set_splitted_nodes_poses(const std::pair<ImRect, ImRect>& poses) noexcept;
    void adjust_r_pose();

#ifdef _DEBUG
    const RerouteData& get_reroute_data() const noexcept { return *m_data; }
#endif

private:
    void get_pins_from_link(const PinInstance* p1, const PinInstance* p2, const PinInstance*& external, const LinkedPair*& pair) const noexcept;
    void get_pins_from_link(const PinInstance* p1, const PinInstance* p2, const PinInstance*& external, LinkedPair*& pair) noexcept;
    void cleanup_pairs(bool keep_linked);
    void refresh_pair_state(LinkedPair& pair);
    PinInstance* add_pin_internal(Workflow* wf, const std::string& type, const std::string& name, int32_t out_index, iid id, hid hash);
    void remove_pin_internal(Workflow* wf, PinInstance* pin, bool force_cleanup) noexcept;
    void refresh_names();
    void propagate_pair_type(const LinkedPair& pair);
    bool pair_is_hidden(const LinkedPair& pair) const noexcept;
    void add_pair_if_empty();

private:
    std::shared_ptr<RerouteData> m_data;
    CNI_Reroute* m_splitted_part = nullptr;
    PinInstance* m_splitted_pin = nullptr;
    iid m_splitter_link_id = 0;
    bool m_now_propagading = false;
};

} // namespace data
