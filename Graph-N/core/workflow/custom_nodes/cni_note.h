/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{
class PinInstance;

struct CNI_DeserializedNote final : public CNI_DeserializedData
{
    Primitive widget_value;
    iid id = 0;
};

class CNI_NoteBase : public CNI_BaseObj
{
public:
    CNI_NoteBase(Node* obj, wid workflow_id, iid instance_id) noexcept;
    ~CNI_NoteBase() noexcept override = default;

    void cni_cleanup_base_object() noexcept override;
    bool cni_is_applicable() const noexcept override { return true; }
    void cni_init() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
};

class CNI_Note final : public CNI_NoteBase
{
public:
    using CNI_NoteBase::CNI_NoteBase;
    ~CNI_Note() noexcept override = default;
};

struct CNI_MarkdownNote final : public CNI_NoteBase
{
public:
    using CNI_NoteBase::CNI_NoteBase;
    ~CNI_MarkdownNote() noexcept override = default;
};

} // namespace data
