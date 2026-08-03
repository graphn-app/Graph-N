#pragma once
#include "ui/dialogs/options/opts_defs.h"

namespace ui
{

class OptRightPane final
{
public:
    OptRightPane() noexcept = default;
    ~OptRightPane() noexcept;

    NO_COPY_MOVE(OptRightPane);

    void init(OptsData& data);
    void init_sizes() noexcept;

    void show(OptsData& data);

private:
    void show_general(const char*& help_text);
    void show_rendering(const char*& help_text);
    void show_connection(const char*& help_text);
    void show_local_server(const char*& help_text);
    void show_widget_input(const char*& help_text);
    void show_canvas(const char*& help_text);
    void show_activity_bar(const char*& help_text);
    void show_thresholds(const char*& help_text);
    void show_keys(const char*& help_text);

    void show_help_mark(const char*& help_text_ptr, const char* hint);
    void show_help_text(const char* help_text);
    bool show_full_row_button(const char* text, bool enabled, bool half_w, bool same_line);

    void import_from_comfyui();
    void browse_for_python();
    void browse_for_mainpy();
    void browse_for_workdir();
    void browse_for_outputdir();

    void load_hotkeys(bool defaults);
    int32_t hotkey_to_cb_index(ImGuiKey key) const noexcept;
    ImGuiKey hotkey_to_imgui_key(int32_t cb_index) const noexcept;

private:
    OptsData* m_data = nullptr;
    ImU32 m_clr_subsep = 0;
};

}
