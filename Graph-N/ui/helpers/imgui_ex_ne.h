#pragma once

namespace data
{
class NodeInstance;
class PinInstance;
}

namespace ImGuiExNe
{

struct DeferredAction final
{
    struct Combo final
    {
        bool is_active = false;
        bool do_open = false;
        bool is_changed = false;
        int64_t old_value = -1;
        ImVec2 screen_pos;
        float width = 0.0f;
        std::function<bool(bool)> draw_foo;
    }
    combo;

    struct MlInput final
    {
        bool is_active = false;
        const char* hint = nullptr;
        std::string* buffer = nullptr;
    }
    ml_input;

    data::NodeInstance* active_node = nullptr;
    data::PinInstance* active_pin = nullptr;
};

inline DeferredAction g_combo_deferred_action;
inline void* g_hovered_item = nullptr;

void Combo(DeferredAction& act, data::NodeInstance* node, data::PinInstance* pin, const char* label, int64_t* current_item, const char* const items[], int64_t items_count) noexcept;
void RenderDeferredCombo(DeferredAction& act);

bool InputTextWithHint(DeferredAction& act, data::NodeInstance* node, data::PinInstance* pin, const char* label, const char* hint, std::string* str) noexcept;

} // namespace ImGuiExNe
