#pragma once

// TODO: remove not needd

namespace ImGuiEx
{

bool TooltipOnLastControl(const char* tt, const char* tt_end, ImGuiHoveredFlags_ delay = ImGuiHoveredFlags_DelayShort,
    bool only_if_modifier_pressed = false, ImGuiKey modifier = ImGuiMod_Ctrl);

void Spacing(size_t count);

void Separator(const char* text);
void Separator(uint32_t color, const char* text = "");
void Separator(uint32_t color, float width, const char* text, size_t text_len = 0, float text_width = 0.0f);

void PushDlgTitleStyles(const uint32_t color);
void PopDlgTitleStyles();

void HelpMarkerOnLastControl(const char* tooltip);
void HelpMarker(const char* tooltip, const char* help_mark, bool same_line = true);

bool CheckMultiselectControls();

void Label(const char* label, float x_offset, const char* text);
void Label(const char* label, float x_offset, const std::string& text);

bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextMultilineWithHint(const char* label, const char* hint, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

ImVec2 CalcTextSizeForMultilineWithHScroll(const std::string& text);

// return pair.first: text edited
// return pair.second: text edition finished (edit controls deactivated)
std::pair<bool, bool> InputTextMultilineWithHScroll(
    const char* text_label, const char* parent_block_label,
    std::string* str,
    float max_widget_w, float max_widget_h,
    const ImVec2& text_sz,
    ImGuiInputTextFlags flags = 0);
std::pair<bool, bool> InputTextMultilineWithHintWithHScroll(
    const char* text_label, const char* parent_block_label,
    const char* hint, std::string* str,
    float max_widget_w, float max_widget_h,
    const ImVec2& text_sz,
    ImGuiInputTextFlags flags = 0);

void ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button);

bool InputIntSmart(const char* label_id, int64_t* value, int64_t step, int64_t min, int64_t max, const std::string& format_named, const std::string& format_unnamed);
bool InputUIntSmart(const char* label_id, uint64_t* value, uint64_t step, uint64_t min, uint64_t max, const std::string& format_named, const std::string& format_unnamed);
bool InputDoubleSmart(const char* label_id,  double* value, double step, double min, double max, const std::string& format_named, const std::string& format_unnamed);

bool DragDoubleSmart(const char* label, double* value, double min, double max, const char* format);
bool DragIntSmart(const char* label, int64_t* value, int64_t min, int64_t max, const char* format);
bool DragUIntSmart(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format);

bool SliderDoubleSmart(const char* label, double* value, double min, double max, const char* format, double step = 0);
bool SliderIntSmart(const char* label, int64_t* value, int64_t min, int64_t max, const char* format, int64_t step = 0);
bool SliderUIntSmart(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format, uint64_t step = 0);

// two regular left/right buttons
bool BooleanButtons1(const char* id_label_on, const char* id_label_off, bool* value, ImVec2 size_arg = ImVec2(0, 0));

// two regular left/right buttons, but single centered text
bool BooleanButtons2(const char* id_label, const char* label, bool* value);

// text at the left, oval-switcher at the right
bool BooleanSwitch(const char* id_label, const char* label, bool* value, float width = 0.0f);

// bounded circle at the right
bool BooleanToggleLED(const char* id_label, bool* value, bool draw_frame, bool left_rounded, float width = 0.0f);

bool UrlButton(const char* id_label, ImU32 url_clr_def, ImU32 url_clr_hover, const char* tooltip);

struct Toast
{
    std::string message;
    float duration = 0.0f;
    ImVec2 pos = {};
    double start_time = 0.0f;
    bool at_cursor_pos = true;
    bool expired = true;
};
Toast toast_prepare(const char* text, float duration_in_ms = 750.0f, bool at_cursor = false, bool increase_wnd = true);
void toast_show(Toast& toast);

} // namespace ImGuiEx
