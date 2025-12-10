#pragma once

// TODO: remove not needd

namespace tools
{
namespace time
{
class dt;
}
}

namespace ImGui
{

void PushGID(gid id);
void PushLID(lid id);

void Spacing(size_t count);

void Separator(const char* text);
void Separator(uint32_t color, const char* text = "");

void PushDlgTitleStyles(const uint32_t color);
void PopDlgTitleStyles();

void HelpMarkerOnLastControl(const char* desc);
void HelpMarker(const char* desc, const char* text = "[?]", bool same_line = true);

bool CheckMultiselectControls();

// return -1 if canceled
// return 0 if in progress
// return 1 if selected
int DatePicker(const char* label, tools::time::dt& date, uint16_t min_year, uint16_t max_year, bool& year_enabled, bool allow_disable_year = true);
int DatePickerRaw(const char* label, tools::time::dt& date, uint16_t min_year, uint16_t max_year, bool& year_enabled, bool allow_disable_year = true);

bool DatePickerText(const std::string& label, const tools::time::dt& date, tools::time::dt& buffer, uint16_t min_year, uint16_t max_year, int month_text_length = 3);

void Label(const char* label, float x_offset, const char* text);
void Label(const char* label, float x_offset, const std::string& text);

bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

void ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button);

} // namespace ImGui
