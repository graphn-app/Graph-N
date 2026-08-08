#pragma once
#include "core/workflow/node_instance.h"
#include "core/workflow/node.h"

namespace mngr
{
class NodesManager;
}

namespace data
{

class CNI_BaseObj
    : public NodeInstance
{
public:
    using NodeInstance::NodeInstance;
    ~CNI_BaseObj() = default;

    void init() override;
    void init(const std::string& select) override;
    void init(const NodeInstance* copy_from_node, bool copy_pin_ids) override;

    virtual void cni_init() {}

    void cni_recreate_base_object();

    static void cni_add_pin(std::unique_ptr<Pin>&& pin) { m_pins.push_back(std::move(pin)); }
    static void cni_remove_pin(Pin* pin) noexcept { m_pins.remove_if([pin](const std::unique_ptr<Pin>& element) noexcept { return pin == element.get(); }); }

protected:
    virtual std::unique_ptr<Node> cni_make_base_object(mngr::NodesManager* /*manager*/) const { return nullptr; }

private:
    std::unique_ptr<Node> m_base_obj;
    static std::list<std::unique_ptr<Pin>> m_pins;
};

template <typename T>
class CNI_BaseObj_T : public CNI_BaseObj
{
public:
    using CNI_BaseObj::CNI_BaseObj;
    ~CNI_BaseObj_T() = default;

    std::unique_ptr<Node> cni_make_base_object(mngr::NodesManager* manager) const override
    {
        return T::cni_create_object(manager);
    }
    static bool cni_is_same(const NodeInstance* node) noexcept
    {
        return strcmp(T::cni_get_node_class(), node->cni_get_node_class()) == 0;
    }
    static bool cni_is_same(const Node* node) noexcept
    {
        return T::cni_get_node_class() == node->node_class;
    }

protected:
    const char* cni_get_node_class() const noexcept override
    {
        return T::cni_get_node_class();
    }
};

} // namespace data
