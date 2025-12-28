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

void Spacing(size_t count);

void Separator(const char* text);
void Separator(uint32_t color, const char* text = "");

void PushDlgTitleStyles(const uint32_t color);
void PopDlgTitleStyles();

void HelpMarkerOnLastControl(const char* tooltip);
void HelpMarker(const char* tooltip, const char* help_mark = ICON_LC_MESSAGE_CIRCLE_QUESTION_MARK, bool same_line = true);

bool CheckMultiselectControls();

void Label(const char* label, float x_offset, const char* text);
void Label(const char* label, float x_offset, const std::string& text);

bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

void ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button);

} // namespace ImGui
