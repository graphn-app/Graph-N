
#include "core/data/node.h"
#include "core/managers/settings_manager.h"

namespace data
{

Node::~Node() = default;

void Node::finalize(std::unordered_map<std::string, std::list<hid>>& models_map)
{
    w_name = ImGui::CalcTextSize(display_name.empty() ? name.c_str() : display_name.c_str()).x;

    const auto scan_pins_for_models = [this](const Input& input, std::unordered_map<std::string, std::list<hid>>& models_map)
    {
        for (auto pin : input.pins)
        {
            if (pin->def_state.type != PinType::COMBO)
                continue;

            for (auto& item : pin->list)
            {
                if (std::holds_alternative<std::string>(item))
                {
                    models_map[*Pin::get_string(item)].push_back(get_id());
                }
            }
        }
    };
    scan_pins_for_models(required, models_map);
    scan_pins_for_models(optional, models_map);
}

Pin* Node::find_pin(hid pin_id, bool input)
{
    const auto find_pin_in = [](hid pin_id, const std::vector<Pin*>& pins) -> Pin*
    {
        for (auto pin : pins)
        {
            if (pin->get_id() == pin_id)
                return pin;
        }
        return nullptr;
    };

    if (input)
    {
        auto pin = find_pin_in(pin_id, required.pins);
        if (nullptr == pin)
        {
            pin = find_pin_in(pin_id, optional.pins);
            if (nullptr == pin)
            {
                pin = find_pin_in(pin_id, hidden.pins);
            }
        }
        return pin;
    }
    return find_pin_in(pin_id, output);
}



} // namespace data
