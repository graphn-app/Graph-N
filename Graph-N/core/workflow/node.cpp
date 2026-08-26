
#include "core/workflow/node.h"
#include "core/managers/settings_manager.h"

namespace data
{

Node::Node(const std::string& class_name)
    : base::HashID(class_name)
{
    name = node_class = display_name = class_name;
    flags.set(NodeObjFlags::CanBypass, NodeObjFlags::CanMute, NodeObjFlags::CanRename);
}

Node::~Node() noexcept = default;

void Node::finalize(std::unordered_map<std::string, std::list<hid>>* models_map)
{
    w_name = ImGui::CalcTextSize(display_name.empty() ? name.c_str() : display_name.c_str()).x;

    if (input.empty() || output.empty())
        flags.remove(NodeObjFlags::CanBypass);

    if (nullptr == models_map)
        return;

    for (auto pin : input)
    {
        if (pin->def_state.type != PinType::COMBO)
            continue;

        for (auto& item : pin->combo_items)
        {
            if (item.is_string())
            {
                (*models_map)[item.get_string()].push_back(get_hash());
            }
        }
    }
}

Pin* Node::find_pin(hid pin_hash, bool search_input) noexcept
{
    const auto find_pin = [](hid pin_hash, const std::vector<Pin*>& pins) -> Pin*
    {
        const auto it = std::ranges::find_if(pins, [pin_hash](Pin* pin) { return pin->get_hash() == pin_hash; });
        if (it != pins.end())
            return *it;
        return nullptr;
    };

    if (search_input)
        return find_pin(pin_hash, input);
    return find_pin(pin_hash, output);
}

} // namespace data
