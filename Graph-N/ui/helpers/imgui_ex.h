/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

// TODO: remove not needd

namespace ImGuiEx
{

const char* cut_text_with_ellipsis(const std::string& text, float width, const char*& pend);

ImU32 increase_brightness(ImU32 source) noexcept;
ImU32 decrease_brightness(ImU32 source) noexcept;

bool TooltipOnLastControl(const char* tt, const char* tt_end, ImGuiHoveredFlags_ delay = ImGuiHoveredFlags_DelayShort,
    bool only_if_modifier_pressed = false, ImGuiKey modifier = ImGuiMod_Ctrl) noexcept;
bool TooltipOnLastControl(const std::function<void()>& callback, ImGuiHoveredFlags_ delay = ImGuiHoveredFlags_DelayShort,
    bool only_if_modifier_pressed = false, ImGuiKey modifier = ImGuiMod_Ctrl);

void Spacing(size_t count) noexcept;

void Separator(const char* text) noexcept;
void Separator(uint32_t color, const char* text = "") noexcept;
void Separator(uint32_t sep_color, uint32_t text_color, const char* text) noexcept;
void Separator(uint32_t color, float width, const char* text, size_t text_len = 0, float text_width = 0.0f) noexcept;

void PushDlgTitleStyles(const uint32_t color) noexcept;
void PopDlgTitleStyles() noexcept;

void HelpMarkerOnLastControl(const char* tooltip) noexcept;
void HelpMarker(const char* tooltip, const char* help_mark, bool same_line = true) noexcept;

bool CheckMultiselectControls() noexcept;

void Label(const char* label, float x_offset, const char* text) noexcept;
void Label(const char* label, float x_offset, const std::string& text) noexcept;

bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextMultilineWithHint(const char* label, const char* hint, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

ImVec2 CalcTextSizeForMultilineWithHScroll(const std::string& text) noexcept;

// return pair.first: text edited
// return pair.second: text edition finished (edit controls deactivated)
std::pair<bool, bool> InputTextMultilineWithHScroll(
    const char* text_label, ImGuiID parent_block_id,
    std::string* str,
    float max_widget_w, float max_widget_h,
    const ImVec2& text_sz,
    ImGuiInputTextFlags flags = 0);
std::pair<bool, bool> InputTextMultilineWithHintWithHScroll(
    const char* text_label, ImGuiID parent_block_id,
    const char* hint, std::string* str,
    float max_widget_w, float max_widget_h,
    const ImVec2& text_sz,
    ImGuiInputTextFlags flags = 0);

void ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button) noexcept;

bool InputIntSmart(const char* label_id, int64_t* value, int64_t step, int64_t min, int64_t max, const std::string& format_named, const std::string& format_unnamed) noexcept;
//bool InputUIntSmart(const char* label_id, uint64_t* value, uint64_t step, uint64_t min, uint64_t max, const std::string& format_named, const std::string& format_unnamed) noexcept;
bool InputDoubleSmart(const char* label_id,  double* value, double step, double min, double max, const std::string& format_named, const std::string& format_unnamed) noexcept;

bool DragDoubleSmart(const char* label, double* value, double min, double max, const char* format) noexcept;
bool DragIntSmart(const char* label, int64_t* value, int64_t min, int64_t max, const char* format) noexcept;
bool DragDoubleRange(ImGuiID id, double* v_current_min, double* v_current_max, double v_min, double v_max, const char* format_min, const char* format_max) noexcept;
bool DragIntRange(ImGuiID id, int64_t* v_current_min, int64_t* v_current_max, int64_t v_min, int64_t v_max, const char* format_min, const char* format_max) noexcept;
//bool DragUIntSmart(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format) noexcept;

bool SliderDoubleSmart(const char* label, double* value, double min, double max, const char* format, double step = 0) noexcept;
bool SliderIntSmart(const char* label, int64_t* value, int64_t min, int64_t max, const char* format, int64_t step = 0) noexcept;
//bool SliderUIntSmart(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format, uint64_t step = 0) noexcept;

// two regular left/right buttons
bool BooleanButtons1(const char* id_label_on, const char* id_label_off, bool* value, ImVec2 size_arg = ImVec2(0, 0)) noexcept;

// two regular left/right buttons, but single centered text
bool BooleanButtons2(const char* id_label, const char* label, bool* value) noexcept;

// text at the left, oval-switcher at the right
bool BooleanSwitch(const char* id_label, const char* label, bool* value, float width = 0.0f) noexcept;

// bounded circle at the right
bool BooleanToggleLED(const char* id_label, bool* value, bool draw_frame, bool left_rounded, float width = 0.0f) noexcept;

bool UrlButton(const char* id_label, const char* tooltip = nullptr, ImU32 url_clr_def = 0, ImU32 url_clr_hover = 0, bool underline = true) noexcept;

struct Toast final
{
    std::string message;
    float duration = 0.0f;
    ImVec2 pos = {};
    double start_time = 0.0f;
    bool at_cursor_pos = true;
    bool expired = true;
    ImU32 frame_clr = 0;
};
Toast toast_prepare(const char* text, ImU32 frame_clr = 0, float duration_in_ms = 750.0f, bool at_cursor = false, bool increase_wnd = true, const ImVec2* child_pos = nullptr, const ImVec2* child_size = nullptr);
void toast_show(Toast& toast) noexcept;

void IndeterminateProgressBar(const ImVec2& size_arg) noexcept;

} // namespace ImGuiEx
