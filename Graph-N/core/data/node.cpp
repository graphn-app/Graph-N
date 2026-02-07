
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

} // namespace data
