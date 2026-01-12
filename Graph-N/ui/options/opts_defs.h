
#pragma once

enum class OptPane
{
    _none,

    comfyui,
    workflow,
    stats,

    _count,
    _first = _none + 1,
    _last = _count - 1,
    _zero = _none
};

struct OptsData final
{
    NO_COPY_MOVE_CTORS(OptsData);

    const uint32_t caption_separator_clr = IM_COL32(0, 128, 128, 255);
    const uint32_t separator_clr = IM_COL32(0, 180, 180, 255);
    const float input_offset_x = 130;
    int64_t step = 1;
    std::string buffer;

    struct Pane final
    {
        NO_COPY_MOVE_CTORS(Pane);

        bool show_inner_child = false;
        OptPane left_selected_item = OptPane::_none;
    }
    pane;

    struct ComfyUI final
    {
        NO_COPY_MOVE_CTORS(ComfyUI);

        std::string host;
        int64_t port = 0;
        int64_t conn_time = 0;
        int64_t read_time = 0;
        std::string path;
        std::string cmd;
        bool start = false;
        bool stop = false;
        bool hide = false;
        bool capture = false;
        bool parse = false;
    }
    comfyui;


    struct Workflow final
    {
        NO_COPY_MOVE_CTORS(Workflow);

        double d_sample = 42.001;
        uint8_t b_sample = 1;
        WidgetInput numbers = WidgetInput::slider;
        WidgetInput booleans = WidgetInput::slider;

        std::deque<bool> hovered_items;
    }
    wf;
};
