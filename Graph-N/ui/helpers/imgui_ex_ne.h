#pragma once

namespace ImGuiExNe
{

struct DeferredAction final
{
    struct Combo final
    {
        bool is_active = false;
        bool do_open = false;
        ImVec2 screen_pos;
        float width = 0.0f;
        std::function<bool(bool)> draw_foo;
    }
    combo;

    void* active_node = nullptr;
    void* active_pin = nullptr;
};

inline DeferredAction g_combo_deferred_action;
inline void* g_hovered_item = nullptr;

void Combo(DeferredAction& act, void* node_id, void* pin_id, const char* label, int32_t* current_item, const char* const items[], int32_t items_count);
void RenderDeferredCombo(DeferredAction& act);

bool InputTextWithHint(DeferredAction& act, void* node_id, void* pin_id, const char* label, const char* hint, const char* str);

} // namespace ImGuiExNe
