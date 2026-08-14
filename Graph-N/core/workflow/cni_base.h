#pragma once
#include "core/workflow/instance.h"

namespace data
{

struct CNI_DeserializedData
{
    CNI_DeserializedData() noexcept = default;
    virtual ~CNI_DeserializedData() noexcept = default;
};

class CNI_Base : public Instance<Node>
{
protected:
    enum CniFlags
    {
        CNI_Flag_None                   = 0x0000,
        CNI_Flag_Cni                    = 0x0001,
        CNI_Flag_Reroute                = 0x0002,
        CNI_Flag_RerouteSplitted        = 0x0004,
        CNI_Flag_CniCustomWidget        = 0x0008
    };
    void set_cni_flags(uint32_t flags) noexcept { m_cni_flags = flags; }

public:
    using Instance<data::Node>::Instance;
    ~CNI_Base() noexcept override = default;

    virtual void cni_init() {}

    // is this is CNI node
    bool is_cni() const noexcept { return (m_cni_flags & CNI_Flag_Cni) > 0; }

    // additional custom widget is needed
    bool is_cni_custom_widget() const noexcept { return (m_cni_flags & CNI_Flag_CniCustomWidget) > 0; }

    // is reroute
    bool is_cni_reroute() const noexcept { return (m_cni_flags & (CNI_Flag_Cni | CNI_Flag_Reroute)) > 0; }
    bool is_cni_reroute_splitted() const noexcept { return (m_cni_flags & (CNI_Flag_Cni | CNI_Flag_Reroute | CNI_Flag_RerouteSplitted)) > 0; }

    // cni node custom class
    virtual const char* cni_get_node_class() const noexcept { return ""; };

    // remove internal pins
    virtual void cni_cleanup_base_object() {}

    // on creation: is this node meet CNI requirements
    virtual bool cni_is_applicable() const noexcept { return false; }

    // special drawing for CNI node
    virtual void cni_draw_widget(PinInstance* /*pin*/, float /*pos*/, float /*width*/) { LogRError("Must not be called", ); }

    // collect some interesting searchable data
    virtual void cni_collect_search_data(std::stringstream& /*data*/) const {}

    // can link two pins. is_cni() must be called first
    virtual bool cni_can_link(const PinInstance* /*p1*/, const PinInstance* /*p2*/) const noexcept { LogRError("Must not be called", false); };

    // link/unlink handling
    virtual void cni_pin_linked(PinInstance* /*p1*/, PinInstance* /*p2*/) {  LogRError("Must not be called", ); }
    virtual void cni_pin_unlinked(const PinInstance* /*p1*/, const PinInstance* /*p2*/) {  LogRError("Must not be called", ); }

    // copy pins from another node
    virtual bool cni_copy_from(const NodeInstance* /*copy_from_node*/, bool /*copy_pin_ids*/) { return false; }

    // pin value changed, CNI node to return list of pins to mirror value
    virtual void cni_mirror_value_to(const PinInstance* /*src*/, std::vector<PinInstance*>& /*append_pins*/) const { LogRError("Must not be called", ); }

    // all internal objects to be cleaned up
    virtual std::unordered_set<uint64_t> cni_get_all_eds() { return {}; }

    // serialization

    // notify node
    virtual void cni_deserialilzation_begins() {}
    virtual void cni_deserialilzation_ends() {}

    // should export node to API json
    virtual bool cni_json_api_should_export() const noexcept { return true; }

    // JSON export
    virtual bool cni_json_need_custom_serialization() const noexcept { return false; }

    // some deserialized data that should be saved for later use
    void cni_save_deserialized_info(std::unique_ptr<CNI_DeserializedData>&& data) noexcept
    {
        m_deserialized_values = std::move(data);
    }

protected:
    std::unique_ptr<CNI_DeserializedData> m_deserialized_values;
    uint32_t m_cni_flags = 0;
};

} // namespace data
