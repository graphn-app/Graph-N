/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{

struct CNI_DeserializedFastActionRgthree final : public CNI_DeserializedData
{
    std::vector<std::tuple<iid, hid, uint8_t>> in_ids;
    iid out_id = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_FastSwitcherBaseRgthree                            ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

class CNI_FastSwitcherBaseRgthree : public CNI_BaseObj
{
public:
    virtual void refresh_switchers() = 0;
    virtual std::list<std::pair<iid, NodeInstFlags>> switch_state(const PinInstance* widget) = 0;
    virtual CNI_DeserializedFastActionRgthree dump() const noexcept = 0;
    virtual iid node_from_widget(const PinInstance* widget) const noexcept = 0;

protected:
    using CNI_BaseObj::CNI_BaseObj;
    ~CNI_FastSwitcherBaseRgthree() noexcept = default;
};

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_FastModeSwitcherRgthree                            ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

class CNI_FastModeSwitcherRgthree : public CNI_FastSwitcherBaseRgthree
{
public:
    void cni_init() override;
    bool cni_is_applicable() const noexcept override { return true; }

    void refresh_switchers() override;
    std::list<std::pair<iid, NodeInstFlags>> switch_state(const PinInstance* widget) override;

    bool cni_can_link(const PinInstance* /*p1*/, const PinInstance* /*p2*/) const noexcept override { return true; }
    void cni_pin_linked(PinInstance* p1, PinInstance* p2) override;
    void cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2) override;

    void cni_cleanup_base_object() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
    void on_deserialization_ends() override;

    CNI_DeserializedFastActionRgthree dump() const noexcept override;
    iid node_from_widget(const PinInstance* widget) const noexcept override;

protected:
    CNI_FastModeSwitcherRgthree(Node* obj, wid workflow_id, iid instance_id, NodeInstFlags action_mode) noexcept;
    ~CNI_FastModeSwitcherRgthree() noexcept = default;

private:
    void add_pin();

private:
    NodeInstFlags m_action_mode = NodeInstFlags::None;

    struct Switch
    {
        iid linked_node = 0;
        PinInstance* pin = nullptr;
        PinInstance* widget = nullptr;
        NodeInstFlags state;
    };
    std::vector<Switch> m_switches;
};

/////////        CNI_FastBypasserRgthree

class CNI_FastBypasserRgthree final : public CNI_FastModeSwitcherRgthree
{
public:
    CNI_FastBypasserRgthree(Node* obj, wid workflow_id, iid instance_id) noexcept
        : CNI_FastModeSwitcherRgthree(obj, workflow_id, instance_id, NodeInstFlags::ModeBypass)
    {}
    ~CNI_FastBypasserRgthree() noexcept = default;
};

/////////        CNI_FastMuterRgthree

class CNI_FastMuterRgthree final : public CNI_FastModeSwitcherRgthree
{
public:
    CNI_FastMuterRgthree(Node* obj, wid workflow_id, iid instance_id) noexcept
        : CNI_FastModeSwitcherRgthree(obj, workflow_id, instance_id, NodeInstFlags::ModeMute)
    {}
    ~CNI_FastMuterRgthree() noexcept = default;
};

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_FastGroupModeSwitcherRgthree                       ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

class CNI_FastGroupModeSwitcherRgthree : public CNI_FastSwitcherBaseRgthree
{
public:
    void cni_init() override;
    bool cni_is_applicable() const noexcept override { return true; }

    void refresh_switchers() override;
    std::list<std::pair<iid, NodeInstFlags>> switch_state(const PinInstance* widget) override;

    bool cni_can_link(const PinInstance* /*p1*/, const PinInstance* /*p2*/) const noexcept override { return true; }
    void cni_pin_linked(PinInstance* /*p1*/, PinInstance* /*p2*/) noexcept override { }
    void cni_pin_unlinked(const PinInstance* /*p1*/, const PinInstance* /*p2*/) noexcept override { }

    void cni_cleanup_base_object() override;
    bool cni_copy_from(const NodeInstance* /*copy_from_node*/, bool /*copy_pin_ids*/) override { return true; }
    void on_deserialization_ends() override;

    CNI_DeserializedFastActionRgthree dump() const noexcept override;
    iid node_from_widget(const PinInstance* widget) const noexcept override;

protected:
    CNI_FastGroupModeSwitcherRgthree(Node* obj, wid workflow_id, iid instance_id, NodeInstFlags action_mode) noexcept;
    ~CNI_FastGroupModeSwitcherRgthree() noexcept = default;

private:
    struct Switch
    {
        iid linked_group = 0;
        PinInstance* widget = nullptr;
        NodeInstFlags state;
    };

    void add_widget(NodeInstance* group);
    void delete_widget(Switch sw);
    bool get_group_state(NodeInstance* group) const;

private:
    NodeInstFlags m_action_mode = NodeInstFlags::None;
    std::list<Switch> m_switches;
};

///////        CNI_FastGroupBypasserRgthree

class CNI_FastGroupBypasserRgthree final : public CNI_FastGroupModeSwitcherRgthree
{
public:
    CNI_FastGroupBypasserRgthree(Node* obj, wid workflow_id, iid instance_id) noexcept
        : CNI_FastGroupModeSwitcherRgthree(obj, workflow_id, instance_id, NodeInstFlags::ModeBypass)
    {}
    ~CNI_FastGroupBypasserRgthree() noexcept = default;
};

///////        CNI_FastGroupMuterRgthree

class CNI_FastGroupMuterRgthree final : public CNI_FastGroupModeSwitcherRgthree
{
public:
    CNI_FastGroupMuterRgthree(Node* obj, wid workflow_id, iid instance_id) noexcept
        : CNI_FastGroupModeSwitcherRgthree(obj, workflow_id, instance_id, NodeInstFlags::ModeMute)
    {}
    ~CNI_FastGroupMuterRgthree() noexcept = default;
};

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_FastActionRgthree                                  ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

class CNI_FastActionRgthree : public CNI_FastSwitcherBaseRgthree
{
public:
    CNI_FastActionRgthree(Node* obj, wid workflow_id, iid instance_id) noexcept;
    ~CNI_FastActionRgthree() noexcept = default;

    void cni_init() override;
    bool cni_is_applicable() const noexcept override { return true; }

    void refresh_switchers() override;
    std::list<std::pair<iid, NodeInstFlags>> switch_state(const PinInstance* widget) override;

    bool cni_can_link(const PinInstance* /*p1*/, const PinInstance* /*p2*/) const noexcept override { return true; }
    void cni_pin_linked(PinInstance* p1, PinInstance* p2) override;
    void cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2) override;

    void cni_cleanup_base_object() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
    void on_deserialization_ends() override;

    CNI_DeserializedFastActionRgthree dump() const noexcept override;
    iid node_from_widget(const PinInstance* widget) const noexcept override;

    static uint8_t mode_from_string(const std::string& string_mode) noexcept;
    static std::string string_from_mode(const Primitive& value) noexcept;

    void cni_mirror_value_to(const PinInstance* /*src*/, std::vector<PinInstance*>& /*append_pins*/) const override { }

private:
    void add_pin();

private:
    struct Switch
    {
        iid linked_node = 0;
        PinInstance* pin = nullptr;
        PinInstance* widget = nullptr;
    };
    std::vector<Switch> m_switches;
};

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_BookmarkRgthree                                    ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

struct CNI_DeserializedBookmarkRgthree final : public CNI_DeserializedData
{
    int32_t hotkey = 0;
    float zoom = 1.0;
};

/*
TODO: CNI_BookmarkRgthree

register bookmark
unregister on wf focus change
unregister on wf closure/deletion
unregister on node removal
handle hotkeys
resolve zoom issue
serialization
*/

class CNI_BookmarkRgthree : public CNI_BaseObj
{
public:
    using CNI_BaseObj::CNI_BaseObj;
    ~CNI_BookmarkRgthree() noexcept = default;

    void cni_init() override;
    bool cni_is_applicable() const noexcept override { return true; }

    bool cni_can_link(const PinInstance* /*p1*/, const PinInstance* /*p2*/) const noexcept override { return true; }
    void cni_pin_linked(PinInstance* /*p1*/, PinInstance* /*p2*/) override {}
    void cni_pin_unlinked(const PinInstance* /*p1*/, const PinInstance* /*p2*/) override {}

    void cni_cleanup_base_object() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
    void on_deserialization_ends() override {}

    CNI_DeserializedBookmarkRgthree dump() const noexcept;

    void cni_mirror_value_to(const PinInstance* /*src*/, std::vector<PinInstance*>& /*append_pins*/) const override { }

private:
    int32_t m_hotkey = 0;
    float m_zoom = 1.0f;
};

} // namespace data
