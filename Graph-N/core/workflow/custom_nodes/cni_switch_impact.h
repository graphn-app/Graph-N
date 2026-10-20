/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{

struct CNI_DeserializedSwitchImpact final : public CNI_DeserializedData
{
    std::vector<std::pair<iid, hid>> in_ids;
    std::vector<std::pair<iid, hid>> out_ids;
    std::vector<std::pair<iid, hid>> in_dynamic_ids;
    std::vector<std::pair<iid, hid>> out_dynamic_ids;
    int64_t selected = 1;
    bool mode = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_SwitchImpact                                       ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

class CNI_SwitchImpact : public CNI_BaseObj
{
public:
    CNI_SwitchImpact(Node* obj, wid workflow_id, iid instance_id) noexcept;
    ~CNI_SwitchImpact() noexcept = default;

    void cni_init() override;
    bool cni_is_applicable() const noexcept override { return true; }

    bool cni_can_link(const PinInstance* /*p1*/, const PinInstance* /*p2*/) const noexcept override { return true; }
    void cni_pin_linked(PinInstance* p1, PinInstance* p2) override;
    void cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2) override;

    void cni_cleanup_base_object() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
    void on_deserialization_ends() override {}
    void cni_mirror_value_to(const PinInstance* /*src*/, std::vector<PinInstance*>& /*append_pins*/) const override {}

    CNI_DeserializedSwitchImpact dump() const noexcept;

private:
    PinInstance* add_pin();
    void set_base_type();
    void check_base_type();

private:
    PinType m_base_type = PinType::ANY;
    std::string m_base_type_str = "*";
    PinInstance* m_pin_select = nullptr;
    PinInstance* m_pin_mode = nullptr;
    PinInstance* m_pin_out = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_SwitchInversedImpact                               ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

class CNI_SwitchInversedImpact : public CNI_BaseObj
{
public:
    CNI_SwitchInversedImpact(Node* obj, wid workflow_id, iid instance_id) noexcept;
    ~CNI_SwitchInversedImpact() noexcept = default;

    void cni_init() override;
    bool cni_is_applicable() const noexcept override { return true; }

    bool cni_can_link(const PinInstance* /*p1*/, const PinInstance* /*p2*/) const noexcept override { return true; }
    void cni_pin_linked(PinInstance* p1, PinInstance* p2) override;
    void cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2) override;

    void cni_cleanup_base_object() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
    void on_deserialization_ends() override {}
    void cni_mirror_value_to(const PinInstance* /*src*/, std::vector<PinInstance*>& /*append_pins*/) const override {}

    CNI_DeserializedSwitchImpact dump() const noexcept;

private:
    PinInstance* add_pin();
    void set_base_type();
    void check_base_type();

private:
    PinType m_base_type = PinType::ANY;
    std::string m_base_type_str = "*";
    int32_t m_last_out_index = 0;
    PinInstance* m_pin_in = nullptr;
    PinInstance* m_pin_select = nullptr;
    PinInstance* m_pin_mode = nullptr;
};

} // namespace data
