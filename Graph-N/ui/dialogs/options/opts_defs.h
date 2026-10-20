/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

struct OptsData final
{
    NO_COPY_MOVE_CTORS(OptsData);

    bool enable_always_on_top = true;
    bool show_inner_child = false;
    size_t left_selected_item = cast::to_szt(OptPane::_count);

    float left_pane_w = 0.0f;
    float input_offset_x = 0.0f;
    float input_small_offset_x = 0.0f;
    float input_w = 0.0f;
    float input_small_w = 0.0f;
    float full_row_input_w = 0.0f;
    float input_with_browse_w = 0.0f;
    float full_row_input_with_browse_w = 0.0f;
    float browse_btn_w = 0.0f;
    float help_mark_w = 0.0f;
    
    std::vector<std::string> buffer_s;
    std::vector<int64_t> buffer_i;

    struct KeyBindEx final : public ui::KeyBinding
    {
        int32_t cb_index = 0;
        bool valid = true;

        KeyBindEx() noexcept = default;
        ~KeyBindEx() noexcept = default;
        KeyBindEx(const ui::KeyBinding& kb) noexcept
        {
            target = kb.target;
            enabled = kb.enabled;
            key = kb.key;
            alt = kb.alt;
            cmd = kb.cmd;
            ctrl = kb.ctrl;
            shift = kb.shift;
            node_related = kb.node_related;
            repeatable = kb.repeatable;
        }
        ui::KeyBinding to_keybinds() const noexcept
        {
            ui::KeyBinding kb;
            kb.target = target;
            kb.enabled = enabled;
            kb.key = key;
            kb.alt = alt;
            kb.cmd = cmd;
            kb.ctrl = ctrl;
            kb.shift = shift;
            kb.node_related = node_related;
            kb.repeatable = repeatable;
            return kb;
        }
    };
    std::vector<KeyBindEx> hotkeys;
    std::vector<ImGuiKey> hotkeys_names_k;
    std::vector<const char*> hotkeys_names_n;
    bool hotkeys_changed = false;
    bool hotkeys_valid = true;

    struct Input final
    {
        NO_COPY_MOVE_CTORS(Input);

        double d_sample = 42.001;
        uint8_t b_sample = 1;
        WidgetInput numbers = WidgetInput::Slider;
        WidgetInput booleans = WidgetInput::Slider;
        std::deque<bool> hovered_items;
    }
    input;
};
