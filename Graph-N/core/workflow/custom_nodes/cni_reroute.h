#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{
class PinInstance;

struct RoutedPinPair final
{
    std::string type = "*";
    iid in_id = 0;
    iid out_id = 0;
    std::string in_pin_name = "*";
    int32_t out_index = -1;
    bool hidden = false;
};
struct CNI_DeserializedReroute final : public CNI_DeserializedData
{
    std::vector<RoutedPinPair> pairs;
    std::map<PinType, int32_t> in_pin_types;
    int32_t next_out_index = 0;
};

class CNI_Reroute final
    : public CNI_BaseObj_T<CNI_Reroute>
{
public:
    using CNI_BaseObj_T<CNI_Reroute>::CNI_BaseObj_T;
    ~CNI_Reroute() noexcept = default;

    static std::unique_ptr<Node> cni_create_object(mngr::NodesManager* manager);
    static const char* cni_get_node_class() noexcept { return CNI_CLASS_REROUTE; }

    void cni_init() override;

    bool is_cni() const noexcept override { return true; }
    bool is_cni_reroute() const noexcept override { return true; }
    bool is_cni_reroute_splitted() const noexcept override { return m_splitted; }

    bool cni_is_applicable() const noexcept override { return true; }

    bool cni_can_link(const PinInstance* p1, const PinInstance* p2) const noexcept override;
    void cni_pin_linked(PinInstance* p1, PinInstance* p2) override;
    void cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2) override;

    void cni_cleanup_base_object() override;

    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;

    void cni_mirror_value_to(const PinInstance* src, std::vector<PinInstance*>& append_pins) const override;

    std::unordered_set<uint64_t> cni_get_all_eds() override;

    void cni_deserialilzation_begins() noexcept override;
    void cni_deserialilzation_ends() override;

    bool cni_json_api_should_export() const noexcept override { return false; }
    bool cni_json_need_custom_serialization() const noexcept override { return true; }

private:
    enum class DrawingPart
    {
        Left,
        Right,
    };

public:
    EdType& ed() noexcept override { return !m_splitted || DrawingPart::Left == m_drawing_splitted_part ?  CNI_BaseObj_T<CNI_Reroute>::ed() : m_ed_splitted; }
    const EdType& ed() const noexcept override { return !m_splitted || DrawingPart::Left == m_drawing_splitted_part ?  CNI_BaseObj_T<CNI_Reroute>::ed() : m_ed_splitted; }
    uint64_t reset_state() noexcept override;

    void reroute_split_to_parts() override;
    void reroute_merge_parts(bool to_left) override;

    void reroute_splitted_begin_draw_parts() noexcept override { m_drawing_splitted_part = DrawingPart::Left; }
    bool reroute_splitted_end_draw_parts() noexcept override
    {
        if (!m_splitted)
            return false;
        m_drawing_splitted_part = DrawingPart::Right;
        if (!m_splitted_pos_set)
        {
            m_pos_set = false;
            m_splitted_pos_set = true;
        }
        return true;
    }

    ImVec2 reroute_splitted_get_current_node_pos() const noexcept override
    {
        return (!m_splitted || DrawingPart::Left == m_drawing_splitted_part ? m_rc.Min : m_splitted_pos);
    }
    void reroute_splitted_set_current_node_pos(const ImVec2& pos) noexcept override
    {
        if (!m_splitted || DrawingPart::Left == m_drawing_splitted_part) m_rc.Min = pos;
        else m_splitted_pos = pos;
    }

    PinInstance* reroute_splitted_get_input() const noexcept override
    {
        if (m_splitted && DrawingPart::Right == m_drawing_splitted_part)
            return m_splitted_in;
        return nullptr;
    }
    PinInstance* reroute_splitted_get_output() const noexcept override
    {
        if (m_splitted && DrawingPart::Left == m_drawing_splitted_part)
            return m_splitted_out;
        return nullptr;
    }

private:
    struct LinkedPair;
    void get_pins_from_link(const PinInstance* p1, const PinInstance* p2, const PinInstance*& external, const LinkedPair*& pair) const noexcept;
    void get_pins_from_link(const PinInstance* p1, const PinInstance* p2, const PinInstance*& external, LinkedPair*& pair) noexcept;
    void cleanup_pairs(bool keep_linked);
    void refresh_pair_state(LinkedPair& pair);
    void add_pair(const RoutedPinPair& pair = {});
    PinInstance* add_pin_internal(Workflow* wf, const std::string& type, const std::string& name, int32_t out_index, iid id);
    void remove_pin_internal(Workflow* wf, PinInstance* pin, bool force_cleanup) noexcept;
    std::string create_pin_name(const std::string& type_str);

private:
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
        PinType type = PinType::_none;
        LinkedIn input;
        LinkedOut output;
    };
    std::list<LinkedPair> m_pairs;
    std::map<PinType, int32_t> m_in_pin_types;
    int32_t m_next_out_index = 1;

    bool m_splitted = false;
    DrawingPart m_drawing_splitted_part = DrawingPart::Left;
    EdType m_ed_splitted;
    ImVec2 m_splitted_pos;
    bool m_splitted_pos_set = false;
    PinInstance* m_splitted_in = nullptr;
    PinInstance* m_splitted_out = nullptr;

    std::unordered_set<uint64_t> m_removed;
};

} // namespace data
