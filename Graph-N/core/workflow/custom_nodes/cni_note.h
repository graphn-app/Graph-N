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

template <typename T>
class CNI_NoteBase : public CNI_BaseObj_T<T>
{
public:
    CNI_NoteBase(Node* obj, wid workflow_id, iid instance_id) noexcept;
    ~CNI_NoteBase() noexcept = default;

    void cni_cleanup_base_object() noexcept override;
    bool cni_is_applicable() const noexcept override { return true; }
    void cni_init() override;
    bool cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids) override;
};


class CNI_Note final : public CNI_NoteBase<CNI_Note>
{
public:
    using CNI_NoteBase<CNI_Note>::CNI_NoteBase;
    ~CNI_Note() noexcept = default;

    static std::unique_ptr<Node> cni_create_object();
};


struct CNI_MarkdownNote final : public CNI_NoteBase<CNI_MarkdownNote>
{
public:
    using CNI_NoteBase<CNI_MarkdownNote>::CNI_NoteBase;
    ~CNI_MarkdownNote() noexcept = default;

    static std::unique_ptr<Node> cni_create_object();
};

} // namespace data
