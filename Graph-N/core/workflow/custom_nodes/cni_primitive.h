#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{
class PinInstance;

struct CNI_DeserializedPrimitive final : public CNI_DeserializedData
{
    Primitive widget_value;
    Primitive cag_value;
    std::string out_pin_name = "*";
    std::string out_pin_type = "*";
    iid out_id = 0;
};

class CNI_Primitive final
    : public CNI_BaseObj_T<CNI_Primitive>
{
public:
    CNI_Primitive(Node* obj, wid workflow_id, iid instance_id) noexcept;
    ~CNI_Primitive() noexcept = default;

    static std::unique_ptr<Node> cni_create_object();
    static const char* cni_get_node_class() noexcept { return CNI_CLASS_PRIMITIVE; }

    void cni_cleanup_base_object() noexcept override;
    bool cni_is_applicable() const noexcept override { return true; }
    void cni_init() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;

    bool cni_can_link(const PinInstance* p1, const PinInstance* p2) const noexcept override;
    void cni_pin_linked(PinInstance* p1, PinInstance* p2) override;
    void cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2) override;

    void cni_mirror_value_to(const PinInstance* src, std::vector<PinInstance*>& append_pins) const override;

    std::list<PinType> get_supported_pin_types(PinInstance* pin) const override;

    void on_deserialilzation_ends() override;

private:
    void update();

private:
    std::vector<PinInstance*> m_links; // linked outs
    bool m_linked = false;

    // outputs[0] -> linked type
    // inputs[0] -> input field: string, int, float, bool or combo
    // inputs[1] -> for int, float and combo: cag
};

} // namespace data
