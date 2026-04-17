
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
