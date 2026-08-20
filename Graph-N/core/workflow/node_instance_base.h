
#pragma once
#include "core/workflow/cni_base.h"

enum NodeFlags
{
    NodeFlag_None                   = 0x00000000,
    NodeFlag_CanDelete              = 0x00000001,
    NodeFlag_CNI                    = 0x00000002,
    NodeFlag_Reroute                = 0x00000004,
    NodeFlag_RerouteSplitted        = 0x00000008,
    NodeFlag_RerouteSplitted_R      = 0x00000010,
    NodeFlag_CustomWidget           = 0x00000020,
};

namespace data
{

class PinInstance;
struct Node;

class NodeInstanceBase : public CNI_Base
{
public:
    NodeInstanceBase(Node* obj, wid workflow_id, iid instance_id, uint32_t node_flags)
        : CNI_Base(obj, workflow_id, instance_id)
    {
        m_node_flags = node_flags | NodeFlags::NodeFlag_CanDelete;
    }
    ~NodeInstanceBase() noexcept override;

    void init() override;

    float calc_node_width(bool ro, bool use_big_pin_icon) noexcept;
    float recalc_node_width(float new_width, bool ro, bool use_big_pin_icon) noexcept;
    virtual void reset_width() noexcept;

    virtual const std::string& get_name() const noexcept;
    virtual const std::string& get_name_original() const noexcept;
    virtual void set_name(std::string name) noexcept;

    float get_ideal_width() const noexcept { return m_ideal_w; }

    const std::vector<PinInstance*>& get_inputs() const noexcept { return m_input; }
    const std::vector<PinInstance*>& get_outputs() const noexcept { return m_output; }

    virtual std::list<PinType> get_supported_pin_types(PinInstance* pin) const;

    void add_input(PinInstance* pin);
    void add_output(PinInstance* pin);

    void remove_pin(const PinInstance* pin) noexcept;
    void reset_pins() noexcept;

    void link_cag_pins() noexcept;

public:
    bool test_node_flags(uint32_t flags) const noexcept { return (m_node_flags & flags) == flags; }
    uint32_t get_node_flags() const noexcept { return m_node_flags; }

    bool is_cni_node() const noexcept { return test_node_flags(NodeFlag_CNI); }
    bool is_cni_reroute() const noexcept { return test_node_flags(NodeFlag_Reroute); }
    bool is_cni_reroute_splitted() const noexcept { return test_node_flags(NodeFlag_RerouteSplitted); }
    bool is_cni_reroute_splitted_r() const noexcept { return test_node_flags(NodeFlag_RerouteSplitted_R); }

protected:
    virtual float get_additional_name_width() const noexcept { return 0.0f; }
    virtual float get_additional_node_width() const noexcept { return 0.0f; }
    virtual bool do_need_calc_node_widths() const noexcept { return true; }

    uint64_t reset_state() noexcept override;

protected:
    void set_node_flags(uint32_t flags, bool append) noexcept
    {
        if (append) m_node_flags |= flags;
        else m_node_flags = flags;
    }
    void clear_node_flags(uint32_t flags) noexcept { m_node_flags &= ~flags; }

public:
    float w_node = 0.0f;
    float w_node_name_off = 0.0f;
    float w_name = -1.0f;
    float w_center = 0.0f;
    float w_separator = 0.0f;
    float w_widget_name = 0.0f;
    float w_widget_value = 0.0f;
    float w_pin_in = 0.0f;
    float w_pin_out = 0.0f;

    bool m_has_in_pins = false;
    bool m_has_out_pins = false;
    bool m_has_widgets = false;

protected:
    std::vector<PinInstance*> m_input;
    std::vector<PinInstance*> m_output;
    std::string m_name;
    float m_saved_width = -1.0f;

    // !!
    // check `init` and `reset_state` when adding new field

private:
    float m_ideal_w = 0.0f;
    float m_min_possible_w = 0.0f;
    const float m_min_center_w = 10.0f;

    uint32_t m_node_flags = NodeFlag_None;
};

} // namespace data

