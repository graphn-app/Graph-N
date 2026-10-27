/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{

struct CNI_DeserializedPowerLoraLoaderRgthree final : public CNI_DeserializedData
{
    std::list<std::tuple<std::string /*lora*/, bool /*enabled*/, double /*strength*/>> loras;
};

class CNI_PowerLoraLoaderRgthree final : public CNI_BaseObj
{
public:
    CNI_PowerLoraLoaderRgthree(Node* obj, wid workflow_id, iid instance_id) noexcept;
    ~CNI_PowerLoraLoaderRgthree() noexcept override = default;

    void cni_init() override;
    bool cni_is_applicable() const noexcept override { return true; }

    bool cni_can_link(const PinInstance* /*p1*/, const PinInstance* /*p2*/) const noexcept override { return true; }
    void cni_pin_linked(PinInstance* /*p1*/, PinInstance* /*p2*/) noexcept override {}
    void cni_pin_unlinked(const PinInstance* /*p1*/, const PinInstance* /*p2*/) noexcept override {}
    void cni_mirror_value_to(const PinInstance* /*src*/, std::vector<PinInstance*>& /*append_pins*/) const noexcept override { }

    void cni_cleanup_base_object() noexcept override {}
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
    void on_deserialization_ends() noexcept override;
    float get_additional_node_width() const noexcept override;

    void add_lora(std::string lora, bool enabled, double strength = 1.0, bool add_to_history = false);
    void toggle_all();
    void enable_lora(size_t lora_index, bool enable);
    void set_strength(size_t lora_index, double prev_value);

    struct Lora final
    {
        bool enabled = true;
        double strength = 1.0;
        double strength_backup = 1.0;
        std::string name;
        std::string ext;
    };
    std::vector<Lora>& get_loras() noexcept { return m_loras; }
    const std::vector<Lora>& get_loras() const noexcept { return m_loras; }
    const bool get_toggler() const noexcept { return m_toggler; }

    blob::Blob serialize() const;
    void deserialize(const blob::Blob& blob);

private:
    std::vector<Lora> m_loras;
    bool m_toggler = true;
};

} // namespace data
