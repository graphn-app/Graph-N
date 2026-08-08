
#include "core/workflow/node.h"
#include "core/managers/settings_manager.h"

namespace data
{

Node::Node(const std::string& class_name)
    : base::HashID(class_name)
{
    name = node_class = display_name = class_name;
}

Node::~Node() noexcept = default;

void Node::finalize(std::unordered_map<std::string, std::list<hid>>* models_map)
{
    w_name = ImGui::CalcTextSize(display_name.empty() ? name.c_str() : display_name.c_str()).x;

    if (required.pins.empty() && optional.pins.empty() || output.empty())
        can_bypass = false;

    if (nullptr == models_map)
        return;

    const auto scan_pins_for_models = [this](const Input& input, std::unordered_map<std::string, std::list<hid>>& models_map)
    {
        for (auto pin : input.pins)
        {
            if (pin->def_state.type != PinType::COMBO)
                continue;

            for (auto& item : pin->combo_items)
            {
                if (item.is_string())
                {
                    models_map[item.get_string()].push_back(get_hash());
                }
            }
        }
    };
    scan_pins_for_models(required, *models_map);
    scan_pins_for_models(optional, *models_map);
}

Pin* Node::find_pin(hid pin_hash, bool input) noexcept
{
    const auto find_pin_in = [](hid pin_hash, const std::vector<Pin*>& pins) -> Pin*
    {
        for (auto pin : pins)
        {
            LogRAssertS(pin, "Zero ptr", nullptr);
            if (pin->get_hash() == pin_hash)
                return pin;
        }
        return nullptr;
    };

    if (input)
    {
        auto pin = find_pin_in(pin_hash, required.pins);
        if (nullptr == pin)
        {
            pin = find_pin_in(pin_hash, optional.pins);
            if (nullptr == pin)
            {
                pin = find_pin_in(pin_hash, hidden.pins);
            }
        }
        return pin;
    }
    return find_pin_in(pin_hash, output);
}

} // namespace data
