#include "core/workflow/custom_nodes/cni_base_object.h"

namespace data
{

std::list<std::unique_ptr<Pin>> CNI_BaseObj::m_pins;

void CNI_BaseObj::cni_recreate_base_object()
{
    m_base_obj = cni_make_base_object(nullptr);
    if (m_base_obj)
        set_object(m_base_obj.get());
}

void CNI_BaseObj::init()
{
    cni_recreate_base_object();
    NodeInstanceBase::init();
    cni_init();
}

void CNI_BaseObj::init(const std::string& select)
{
    cni_recreate_base_object();
    NodeInstance::init(select);
    cni_init();
}

void CNI_BaseObj::init(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    cni_recreate_base_object();
    NodeInstance::init(copy_from_node, copy_pin_ids);
    cni_init();
}

} // namespace data
