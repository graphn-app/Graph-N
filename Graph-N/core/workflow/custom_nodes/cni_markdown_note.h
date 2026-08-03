#pragma once
#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{
class PinInstance;

struct CNI_MarkdownNote final
    : public CNI_BaseObj_T<CNI_MarkdownNote>
{
public:
    using CNI_BaseObj_T<CNI_MarkdownNote>::CNI_BaseObj_T;
    ~CNI_MarkdownNote() noexcept = default;

    static std::unique_ptr<Node> cni_create_object(mngr::NodesManager* manager);
    static const char* cni_get_node_class() noexcept { return CNI_CLASS_MARKDOWN_NOTE; }

    void cni_cleanup_base_object() noexcept override;

    bool is_cni() const noexcept override { return true; }
    bool cni_is_applicable() const noexcept override { return true; }
};

} // namespace data
