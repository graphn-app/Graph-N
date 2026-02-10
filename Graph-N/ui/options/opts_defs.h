
#pragma once

struct OptsData final
{
    NO_COPY_MOVE_CTORS(OptsData);

    bool enable_always_on_top = true;

    const float input_offset_x = 150;
    const float help_mark_w = 29.0f;

    std::vector<std::vector<std::string>> buffer_s;
    std::vector<std::vector<int64_t>> buffer_i;

    struct Pane final
    {
        NO_COPY_MOVE_CTORS(Pane);

        bool show_inner_child = false;
        OptPane left_selected_item = OptPane::_none;
    }
    pane;

    struct ComfyUi final
    {
        std::vector<std::string>* buffer_s;
        std::vector<int64_t>* buffer_i;
    }
    comfyui;

    struct Stats final
    {
        std::vector<std::string>* buffer_s;
        std::vector<int64_t>* buffer_i;
    }
    stats;

    struct Workflow final
    {
        NO_COPY_MOVE_CTORS(Workflow);

        double d_sample = 42.001;
        uint8_t b_sample = 1;
        WidgetInput numbers = WidgetInput::Slider;
        WidgetInput booleans = WidgetInput::Slider;

        std::deque<bool> hovered_items;
    }
    wf;
};
