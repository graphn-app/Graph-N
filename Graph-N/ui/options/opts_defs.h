
#pragma once

enum class OptPane
{
    _none,

    comfyui,

    _first = comfyui,
    _last = comfyui,
    _zero = _none
};

struct OptsData final
{
    NO_COPY_MOVE_CTORS(OptsData);

    const uint32_t separator_clr = IM_COL32(0, 180, 180, 255);
    const float input_offset_x = 130;
    int64_t step = 1;
    std::vector<bool> help_marks;

    struct Pane final
    {
        NO_COPY_MOVE_CTORS(Pane);

        bool show_inner_child = false;
        OptPane left_selected_item = OptPane::_none;
    }
    pane;

    struct Comfy final
    {
        NO_COPY_MOVE_CTORS(Comfy);

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
    comfy;
};
