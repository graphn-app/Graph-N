#pragma once
#include "core/workflow/node_instance.h"
#include "core/workflow/node.h"

namespace mngr
{
class NodesManager;
}

namespace data
{

class CNI_BaseObj : public NodeInstance
{
public:
    using NodeInstance::NodeInstance;
    ~CNI_BaseObj() = default;

    void init() override;
    void init(const std::string& select) override;
    void init(const NodeInstance* copy_from_node, bool copy_pin_ids) override;

    void cni_recreate_base_object();
    static std::unique_ptr<Node> cni_make_base_object(hid hash);

    static void cni_add_pin(std::unique_ptr<Pin>&& pin) { m_pins.push_back(std::move(pin)); }
    static void cni_remove_pin(Pin* pin) noexcept { m_pins.remove_if([pin](const std::unique_ptr<Pin>& element) noexcept { return pin == element.get(); }); }

#ifdef _DEBUG
    static std::list<std::unique_ptr<Pin>>& get_all_pins() noexcept { return m_pins; }
#endif

protected:

private:
    std::unique_ptr<Node> m_base_obj;
    static std::list<std::unique_ptr<Pin>> m_pins;
};

} // namespace data
