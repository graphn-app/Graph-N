#pragma once

// TODO: remove not needd

namespace ImGuiEx
{

void Spacing(size_t count);

void Separator(const char* text);
void Separator(uint32_t color, const char* text = "");

void PushDlgTitleStyles(const uint32_t color);
void PopDlgTitleStyles();

void HelpMarkerOnLastControl(const char* tooltip);
void HelpMarker(const char* tooltip, const char* help_mark, bool same_line = true);

bool CheckMultiselectControls();

void Label(const char* label, float x_offset, const char* text);
void Label(const char* label, float x_offset, const std::string& text);

bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

void ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button);

bool InputIntSmart(const char* label_id, 
    int64_t* value, int64_t step, int64_t min, int64_t max,
    const std::string& format_named, const std::string& format_unnamed);

bool InputUIntSmart(const char* label_id,
    uint64_t* value, uint64_t step, uint64_t min, uint64_t max,
    const std::string& format_named, const std::string& format_unnamed);

bool InputDoubleSmart(const char* label_id, 
    double* value, double step, double min, double max,
    const std::string& format_named, const std::string& format_unnamed);

// two regular left/right buttons
bool BooleanButtons1(const char* id_label_on, const char* id_label_off, bool* value, ImVec2 size_arg = ImVec2(0, 0));

// two regular left/right buttons, but single centered text
bool BooleanButtons2(const char* id_label, const char* label, bool* value);

// text at the left, oval-switcher at the right
bool BooleanSwitch(const char* id_label, const char* label, bool* value, float width = 0.0f);

// bounded circle at the right
bool BooleanToggleLED(const char* id_label, bool* value, float width = 0.0f);

} // namespace ImGuiEx
