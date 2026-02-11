#pragma once
#include "ui/options/opts_defs.h"

namespace ui
{

class OptRightPane final
{
public:
    OptRightPane() = default;
    ~OptRightPane();

    NO_COPY_MOVE(OptRightPane);

    void init(OptsData& data);
    void init_sizes();

    void show(OptsData& data);

private:
    void show_general(const char*& help_text);
    void show_rendering(const char*& help_text);
    void show_connection(const char*& help_text);
    void show_local_server(const char*& help_text);
    void show_widget_input(const char*& help_text);
    void show_canvas(const char*& help_text);
    void show_thresholds(const char*& help_text);
    void show_keys(const char*& help_text);

    void show_help_mark(const char*& help_text_ptr, const char* hint);
    void show_help_text(const char* help_text);
    bool show_full_row_button(const char* text, bool enabled);

    void import_from_comfyui();
    void browse_for_python();
    void browse_for_mainpy();
    void browse_for_workdir();
    void browse_for_outputdir();
    void run_server();

private:
    OptsData* m_data = nullptr;
};

}
