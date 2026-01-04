
#include "ui/imgui_ex.h"

namespace ImGui
{

void Spacing(size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        Spacing();
    }
}

void Separator(const char* text)
{
    Spacing();
    SeparatorText(text);
}

void Separator(uint32_t color, const char* text /*= ""*/)
{
    Spacing();
    PushStyleColor(ImGuiCol_Text, color);
    SeparatorText(text);
    PopStyleColor();
}

void PushDlgTitleStyles(const uint32_t color)
{
    PushStyleColor(ImGuiCol_TitleBgActive, color);
    PushStyleColor(ImGuiCol_TitleBg, color);
    PushStyleColor(ImGuiCol_TitleBgCollapsed, color);
}

void PopDlgTitleStyles()
{
    PopStyleColor(3);
}

void HelpMarkerOnLastControl(const char* tooltip)
{
    if (IsItemHovered(ImGuiHoveredFlags_DelayShort) && BeginTooltip())
    {
        PushTextWrapPos(GetFontSize() * 35.0f);
        TextUnformatted(tooltip);
        PopTextWrapPos();
        EndTooltip();
    }
}

void HelpMarker(const char* tooltip, const char* help_mark /*= ICON_LC_MESSAGE_CIRCLE_QUESTION_MARK*/, bool same_line /*= true*/)
{
    if (same_line)
        SameLine();

    ImGuiContext& g = *GImGui;
    PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
    TextUnformatted(help_mark);
    PopStyleColor();

    HelpMarkerOnLastControl(tooltip);
}

bool CheckMultiselectControls()
{
    const int mouse_button = (ImGuiPopupFlags_MouseButtonRight & ImGuiPopupFlags_MouseButtonMask_);
    const bool r_click = (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
    return r_click || GetIO().KeyCtrl || GetIO().KeyShift;
}

void Label(const char* label, float x_offset, const char* text)
{
    TextUnformatted(label);
    SameLine();
    SetCursorPosX(x_offset);
    TextUnformatted(text);
}

void Label(const char* label, float x_offset, const std::string& text)
{
    Label(label, x_offset, text.c_str());
}

struct InputTextCallback_UserData final
{
    std::string*            Str;
    ImGuiInputTextCallback  ChainCallback;
    void*                   ChainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData* data)
{
    InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        std::string* str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char*)str->c_str();
    }
    else if (user_data->ChainCallback)
    {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

bool ImGui::InputText(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

bool ImGui::InputTextMultiline(const char* label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextMultiline(label, (char*)str->c_str(), str->capacity() + 1, size, flags, InputTextCallback, &cb_user_data);
}

bool ImGui::InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextWithHint(label, hint, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

void ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button)
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* window = g.CurrentWindow;
    bool hovered = false;
    bool held = false;
    const ImGuiID id = window->GetID("##scrolldraggingoverlay");
    ImGui::KeepAliveID(id);
    const ImGuiButtonFlags button_flags = (ImGuiMouseButton_Left == mouse_button ? ImGuiButtonFlags_MouseButtonLeft : ImGuiMouseButton_Right == mouse_button ? ImGuiButtonFlags_MouseButtonRight : ImGuiButtonFlags_MouseButtonMiddle);
    if (g.HoveredId == 0) // If nothing hovered so far in the frame (not same as IsAnyItemHovered()!)
        ImGui::ButtonBehavior(window->Rect(), id, &hovered, &held, button_flags);
    if (held && delta.x != 0.0f)
        ImGui::SetScrollX(window, window->Scroll.x + delta.x);
    if (held && delta.y != 0.0f)
        ImGui::SetScrollY(window, window->Scroll.y + delta.y);
}

bool InputIntSmart(
    const char* label_id, 
    int64_t* value, int64_t step, int64_t min, int64_t max,
    const std::string& format_named, const std::string& format_unnamed)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if ((ImGui::GetActiveID() != window->GetID(label_id)))
    {
        ImGui::InputScalar(label_id, ImGuiDataType_S64, value, &step, nullptr, format_named.c_str());
        return false;
    }

    if (!ImGui::InputScalar(label_id, ImGuiDataType_S64, value, &step, nullptr, format_unnamed.c_str()))
        return false;

    if (*value < min) *value = min;
    if (*value > max) *value = max;
    return true;
}

bool InputDoubleSmart(
    const char* label_id, 
    double* value, double step, double min, double max,
    const std::string& format_named, const std::string& format_unnamed)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if ((ImGui::GetActiveID() != window->GetID(label_id)))
    {
        ImGui::InputScalar(label_id, ImGuiDataType_Double, value, &step, nullptr, format_named.c_str());
        return false;
    }

    if (!ImGui::InputScalar(label_id, ImGuiDataType_Double, value, &step, nullptr, format_unnamed.c_str()))
        return false;

    if (*value < min) *value = min;
    if (*value > max) *value = max;
    return true;

    // Select format based on state
    // If editing -> show clean number "%.4f"
    // If idle -> show "prefix %.4f"
    //if (is_editing)
    //    buffer = fmt::format(FMT_COMPILE("%.{}f"), *value);
    //else
    //    buffer = fmt::format(FMT_COMPILE("{} %.{}f"), display_prefix, *value);

    // Draw the input
    // Note: We assume 'step' > 0 to show the +/- buttons as in your screenshot
    //return ImGui::InputDouble(label_id, value, step, 0.0f, buffer.c_str());
}

} // namespace ImGui
