
#include "ui/helpers/imgui_ex.h"
#include "core/managers/settings_manager.h"

namespace ImGuiEx
{

const char* cut_text_with_ellipsis(const std::string& text, float width, const char*& pend)
{
    const char* pbeg = text.c_str();
    pend = pbeg + text.size();

    if (ImGui::CalcTextSize(text.c_str(), pend).x <= width)
        return pbeg;

    const float ellipsis_w = ImGui::CalcTextSize("...").x;
    const char* clip_end = nullptr;
    ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, width - ellipsis_w, pbeg, pend, &clip_end);

    if (clip_end != nullptr && clip_end > pbeg)
    {
        static fmt::memory_buffer fmt_buffer;
        fmt_buffer.clear();

        fmt::format_to(std::back_inserter(fmt_buffer), FMT_STRING("{}..."), std::string_view(pbeg, clip_end - pbeg));
        fmt_buffer.push_back('\0');

        pbeg = fmt_buffer.data();
        pend = pbeg + fmt_buffer.size() - 1;
        return pbeg;
    }
    return pbeg;
}

ImU32 increase_brightness(ImU32 source) noexcept
{
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(source);
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(c.x, c.y, c.z, h, s, v);
    v *= 1.5f;
    v = std::max(v, 1.0f);
    ImGui::ColorConvertHSVtoRGB(h, s, v, c.x, c.y, c.z);
    return ImGui::ColorConvertFloat4ToU32(c);
}

ImU32 decrease_brightness(ImU32 source) noexcept
{
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(source);
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(c.x, c.y, c.z, h, s, v);
    v *= 0.7f;
    ImGui::ColorConvertHSVtoRGB(h, s, v, c.x, c.y, c.z);
    return ImGui::ColorConvertFloat4ToU32(c);
}

bool TooltipOnLastControl(const char* tt, const char* tt_end, ImGuiHoveredFlags_ delay /*= ImGuiHoveredFlags_DelayShort*/,
    bool only_if_modifier_pressed /*= false*/, ImGuiKey modifier /*= ImGuiMod_Ctrl*/) noexcept
{
    if (nullptr == tt || 0 == *tt)
        return false;

    bool show_tt = (only_if_modifier_pressed && ImGui::IsKeyDown(modifier) && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone));
    if (!show_tt && !only_if_modifier_pressed && ImGui::IsItemHovered(delay))
        show_tt = true;

    if (show_tt && ImGui::BeginTooltip())
    {
        ImGui::PushTextWrapPos(300.0f);
        ImGui::TextUnformatted(tt, tt_end);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
        return true;
    }
    return false;
}

bool TooltipOnLastControl(const std::function<void()>& callback, ImGuiHoveredFlags_ delay /*= ImGuiHoveredFlags_DelayShort*/,
    bool only_if_modifier_pressed /*= false*/, ImGuiKey modifier /*= ImGuiMod_Ctrl*/)
{
    bool show_tt = (only_if_modifier_pressed && ImGui::IsKeyDown(modifier) && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone));
    if (!show_tt && !only_if_modifier_pressed && ImGui::IsItemHovered(delay))
        show_tt = true;

    if (show_tt && ImGui::BeginTooltip())
    {
        callback();
        ImGui::EndTooltip();
        return true;
    }
    return false;
}

void Spacing(size_t count) noexcept
{
    const auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    for (size_t i = 0; i < count; i++)
    {
        ImGui::ItemSize(ImVec2(0, 0));
    }
}

void Separator(const char* text) noexcept
{
    const auto pos = ImGui::GetCursorPosX();
    ImGui::Spacing();
    ImGui::SetCursorPosX(pos);
    ImGui::SeparatorText(text);
}

void Separator(uint32_t color, const char* text /*= ""*/) noexcept
{
    const auto pos = ImGui::GetCursorPosX();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, color);
    ImGui::SetCursorPosX(pos);
    ImGui::SeparatorText(text);
    ImGui::PopStyleColor();
}

void Separator(uint32_t sep_color, uint32_t text_color, const char* text) noexcept
{
    const auto pos = ImGui::GetCursorPosX();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, sep_color);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    ImGui::SetCursorPosX(pos);
    ImGui::SeparatorText(text);
    ImGui::PopStyleColor(2);
}

void Separator(uint32_t color, float width, const char* text, size_t text_len /*= 0*/, float text_width /*= 0.0f*/) noexcept
{
    const auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    if (0.0f == width)
        width = ImGui::GetContentRegionAvail().x;
    if (0.0f == text_width)
        text_width = ImGui::CalcTextSize(text).x;
    if (0 == text_len)
        text_len = strlen(text);

    const auto font_size = ImGui::GetFontSize() / 2;

    // Calculate positions
    const ImVec2 sep1_p1(window->DC.CursorPos.x, window->DC.CursorPos.y + font_size);
    ImVec2 sep1_p2(0.0f, sep1_p1.y);
    if (text_width + ImGui::GetStyle().ItemSpacing.x * 2 + 10.0f > width) // skip text
    {
        sep1_p2.x = sep1_p1.x + width;
        window->DrawList->AddLine(sep1_p1, sep1_p2, color, 1.0);
    }
    else
    {
        sep1_p2.x = sep1_p1.x + width / 6;
        const ImVec2 txt_p(sep1_p2.x + ImGui::GetStyle().ItemSpacing.x, sep1_p1.y - font_size);
        const ImVec2 sep2_p1(sep1_p2.x + text_width + ImGui::GetStyle().ItemSpacing.x * 2, sep1_p1.y);
        const ImVec2 sep2_p2(sep1_p1.x + width, sep1_p1.y);

        window->DrawList->AddLine(sep1_p1, sep1_p2, color, 1.0);
        window->DrawList->AddText(txt_p, color, text, text + text_len);
        window->DrawList->AddLine(sep2_p1, sep2_p2, color, 1.0);
    }

    // Advance the cursor (ItemSize handles layout spacing automatically)
    // We use a dummy rect with height = thickness + spacing
    ImGui::ItemSize(ImVec2(width, font_size * 2.0f));
}

void PushDlgTitleStyles(const uint32_t color) noexcept
{
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, color);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, color);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, color);
}

void PopDlgTitleStyles() noexcept
{
    ImGui::PopStyleColor(3);
}

void HelpMarkerOnLastControl(const char* tooltip) noexcept
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(tooltip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void HelpMarker(const char* tooltip, const char* help_mark, bool same_line /*= true*/) noexcept
{
    if (same_line)
        ImGui::SameLine();

    const ImGuiContext& g = *GImGui;
    ImGui::PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
    ImGui::TextUnformatted(help_mark);
    ImGui::PopStyleColor();

    HelpMarkerOnLastControl(tooltip);
}

bool CheckMultiselectControls() noexcept
{
    constexpr int32_t mouse_button = (ImGuiPopupFlags_MouseButtonRight & ImGuiPopupFlags_MouseButtonMask_);
    const bool r_click = (ImGui::IsMouseReleased(mouse_button) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
    return r_click || ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift;
}

void Label(const char* label, float x_offset, const char* text) noexcept
{
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(x_offset);
    ImGui::TextUnformatted(text);
}

void Label(const char* label, float x_offset, const std::string& text) noexcept
{
    Label(label, x_offset, text.c_str());
}

struct InputTextCallback_UserData final
{
    std::string*            Str;
    ImGuiInputTextCallback  ChainCallback;
    void*                   ChainCallbackUserData;
};

namespace
{
int32_t InputTextCallback(ImGuiInputTextCallbackData* data)
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
} // namespace 

bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputTextWithHint(label, hint, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);

    const int32_t str_flags = flags | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_Multiline;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputTextEx(label, nullptr, (char*)str->c_str(), cast::to_i32(str->capacity() + 1), size, str_flags, InputTextCallback, &cb_user_data);
}

bool InputTextMultilineWithHint(const char* label, const char* hint, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);

    const int32_t str_flags = flags | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_Multiline;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputTextEx(label, hint, (char*)str->c_str(), cast::to_i32(str->capacity() + 1), size, str_flags, InputTextCallback, &cb_user_data);
}

ImVec2 CalcTextSizeForMultilineWithHScroll(const std::string& text) noexcept
{
    ImVec2 size = ImGui::CalcTextSize(text.c_str(), text.c_str() + text.size());
    const auto& style = ImGui::GetStyle();
    size.x += style.FramePadding.x * 2.0f;
    size.y += style.FramePadding.y * 2.0f + style.ScrollbarSize + ImGui::GetTextLineHeight();
    return size;
}

std::pair<bool, bool> InputTextMultilineWithHScroll(
    const char* text_label, const char* parent_block_label,
    std::string* str,
    float max_widget_w, float max_widget_h,
    const ImVec2& text_sz,
    ImGuiInputTextFlags flags /*= 0*/)
{
    const ImGuiStyle& style = ImGui::GetStyle();

    float child_h = max_widget_h;
    if (text_sz.x > max_widget_w)
        child_h += style.ScrollbarSize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    bool edited = false, deactivated = false;
    if (ImGui::BeginChild(parent_block_label, ImVec2(max_widget_w, child_h), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        const float inner_w = std::max(text_sz.x, ImGui::GetContentRegionAvail().x);
        const float inner_h = std::max(text_sz.y, child_h); 

        edited = ImGuiEx::InputTextMultiline(text_label, str, 
            ImVec2(inner_w, inner_h), 
            flags | ImGuiInputTextFlags_NoHorizontalScroll);

        deactivated = ImGui::IsItemDeactivatedAfterEdit();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    return std::make_pair(edited, deactivated);
}

std::pair<bool, bool> InputTextMultilineWithHintWithHScroll(
    const char* text_label, const char* parent_block_label,
    const char* hint, std::string* str,
    float max_widget_w, float max_widget_h,
    const ImVec2& text_sz,
    ImGuiInputTextFlags flags /*= 0*/)
{
    const ImGuiStyle& style = ImGui::GetStyle();

    float child_h = max_widget_h;
    if (text_sz.x > max_widget_w)
        child_h += style.ScrollbarSize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    bool edited = false, deactivated = false;
    if (ImGui::BeginChild(parent_block_label, ImVec2(max_widget_w, child_h), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        const float inner_w = std::max(text_sz.x, ImGui::GetContentRegionAvail().x);
        const float inner_h = std::max(text_sz.y, child_h); 

        edited = ImGuiEx::InputTextMultilineWithHint(text_label, hint, str, 
            ImVec2(inner_w, inner_h), 
            flags | ImGuiInputTextFlags_NoHorizontalScroll);

        deactivated = ImGui::IsItemDeactivatedAfterEdit();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    return std::make_pair(edited, deactivated);
}

void ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button) noexcept
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

template <typename TYPE>
bool InputTypeSmart(const char* label_id,
    TYPE* value, TYPE step, TYPE min, TYPE max, ImGuiDataType_ type,
    const std::string& format_named, const std::string& format_unnamed) noexcept
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if ((ImGui::GetActiveID() != window->GetID(label_id)))
    {
        if (!ImGui::InputScalar(label_id, type, value, &step, nullptr, format_named.c_str()))
            return false;
    }
    else if (!ImGui::InputScalar(label_id, type, value, &step, nullptr, format_unnamed.c_str()))
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

bool InputIntSmart(
    const char* label_id, 
    int64_t* value, int64_t step, int64_t min, int64_t max,
    const std::string& format_named, const std::string& format_unnamed) noexcept
{
    return InputTypeSmart(label_id, value, step, min, max, ImGuiDataType_S64, format_named, format_unnamed);
}
//
//bool InputUIntSmart(
//    const char* label_id,
//    uint64_t* value, uint64_t step, uint64_t min, uint64_t max,
//    const std::string& format_named, const std::string& format_unnamed) noexcept
//{
//    return InputTypeSmart(label_id, value, step, min, max, ImGuiDataType_U64, format_named, format_unnamed);
//}

bool InputDoubleSmart(
    const char* label_id, 
    double* value, double step, double min, double max,
    const std::string& format_named, const std::string& format_unnamed) noexcept
{
    return InputTypeSmart(label_id, value, step, min, max, ImGuiDataType_Double, format_named, format_unnamed);
}

template <typename TYPE>
bool DragTypeSmart(const char* label, TYPE* value, TYPE min, TYPE max, ImGuiDataType_ type, const char* format) noexcept
{
    // Calculate the full range
    const TYPE range = max - min;

    // Define sensitivity.
    // This means: "It takes 200 pixels of mouse dragging to go from Min to Max"
    constexpr float drag_pixel_span = 200.0f;

    // Calculate dynamic speed
    float speed = cast::to_float(range / drag_pixel_span);

    // Safety clamp for very small ranges (optional)
    // Ensures we don't get stuck with 0 speed if range is tiny
    if (speed <= 0.001f)
        speed = 0.01f;

    else if (ImGui::IsKeyDown(ImGuiMod_Shift))
        speed = 1.0f;

    // Draw the control
    // Assuming 'ImGui::DragFloat' or similar API
    if (ImGui::DragScalar(label, type, value, speed, &min, &max, format))
    {
        if (*value < min) *value = min;
        if (*value > max) *value = max;
        return true;
    }
    return false;
}

bool DragDoubleSmart(const char* label, double* value, double min, double max, const char* format) noexcept
{
    return DragTypeSmart(label, value, min, max, ImGuiDataType_Double, format);
}

bool DragIntSmart(const char* label, int64_t* value, int64_t min, int64_t max, const char* format) noexcept
{
    return DragTypeSmart(label, value, min, max, ImGuiDataType_S64, format);
}
//
//bool DragUIntSmart(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format) noexcept
//{
//    return DragTypeSmart(label, value, min, max, ImGuiDataType_U64, format);
//}

template <typename TYPE>
bool SliderTypeSmart(const char* label, TYPE* value, TYPE min, TYPE max, ImGuiDataType_ type, const char* format, TYPE step) noexcept
{
    if (ImGui::SliderScalar(label, type, value, &min, &max, format))
    {
        if (*value < min) *value = min;
        if (*value > max) *value = max;

        if (step != 0)
        {
            const auto s = cast::to_float(step);
            const auto v = cast::to_float(*value);
            *value = cast::to<TYPE>(roundf(v / s) * s);
        }
        return true;
    }
    return false;
}

bool SliderDoubleSmart(const char* label, double* value, double min, double max, const char* format, double step /*= 0*/) noexcept
{
    return SliderTypeSmart(label, value, min, max, ImGuiDataType_Double, format, step);
}

bool SliderIntSmart(const char* label, int64_t* value, int64_t min, int64_t max, const char* format, int64_t step /*= 0*/) noexcept
{
    return SliderTypeSmart(label, value, min, max, ImGuiDataType_S64, format, step);
}
//
//bool SliderUIntSmart(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format, uint64_t step /*= 0*/) noexcept
//{
//    return SliderTypeSmart(label, value, min, max, ImGuiDataType_U64, format, step);
//}

bool BooleanButtons1(const char* id_label_on, const char* id_label_off, bool* value, ImVec2 size_arg /*= ImVec2(0, 0)*/) noexcept
{
    bool state_changed = false;

    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

     // Calculate Size
     // If width is 0, use all available content region.
    float width = size_arg.x;
    if (0.0f == width)
        width = ImGui::GetContentRegionAvail().x;

    // Use default button height if not specified
    float height = size_arg.y;
    if (0.0f == height)
        height = ImGui::GetFrameHeight();

    const ImVec2 button_size(width * 0.5f, height);

    // Prepare Colors from Style
    // Active side uses the "ButtonActive" color (usually the accent color of the theme)
    // Inactive side uses the standard "Button" color
    const ImVec4 color_active = style.Colors[ImGuiCol_ButtonActive];
    const ImVec4 color_inactive = style.Colors[ImGuiCol_Button];

    // Setup Layout
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.FrameRounding); // Keep theme rounding

    // ---------------------------------------------------------
    // LEFT BUTTON ("OFF")
    // ---------------------------------------------------------
    const bool is_off = !(*value);

    // Logic: If OFF is the current state (is_off == true), it gets the Active Color.
    ImGui::PushStyleColor(ImGuiCol_Button, is_off ? color_active : color_inactive);

    // Prevent hover/active visual feedback if it's already the active selection
    if (is_off)
    {
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_active);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_active);
    }

    if (ImGui::Button(id_label_on, button_size))
    {
        if (*value) // Only change if it was ON
        { 
            *value = false;
            state_changed = true;
        }
    }

    if (is_off)
        ImGui::PopStyleColor(2);
    ImGui::PopStyleColor(1);

    ImGui::SameLine();

    // ---------------------------------------------------------
    // RIGHT BUTTON ("ON")
    // ---------------------------------------------------------
    const bool is_on = *value;

    ImGui::PushStyleColor(ImGuiCol_Button, is_on ? color_active : color_inactive);
    if (is_on)
    {
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_active);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_active);
    }

    if (ImGui::Button(id_label_off, button_size))
    {
        if (!*value)  // Only change if it was OFF
        { 
            *value = true;
            state_changed = true;
        }
    }

    if (is_on)
        ImGui::PopStyleColor(2);
    ImGui::PopStyleColor(1);

    ImGui::PopStyleVar(2); // Pop Spacing and Rounding

    return state_changed;
}

bool BooleanButtons2(const char* id_label, const char* label, bool* value) noexcept
{
    uint8_t u8val = *value;
    constexpr uint8_t min = 0, max = 1;
    if (false == *value) ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetColorU32(ImGuiCol_TextDisabled));
    const bool clicked = ImGui::SliderScalar(id_label, ImGuiDataType_U8, &u8val, &min, &max, label, ImGuiSliderFlags_NoInput);
    if (false == *value) ImGui::PopStyleColor();
    if (clicked)
    {
        *value = (1 == u8val);
        return true;
    }
    return false;
}

bool BooleanSwitch(const char* id_label, const char* label, bool* value, float width /*= 0.0f*/) noexcept
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImGuiStyle& style = ImGui::GetStyle();

    // Setup geometry
    const float height = ImGui::GetFrameHeight();
    const float radius = height * 0.5f;
    const float toggle_w = radius * 4.0f; // Width is approx 2x diameter (diameter = 2*radius, so width = 4*radius)

    // Calculate total widget width
    const float avail_width = ImGui::GetContentRegionAvail().x;
    const float total_width = (width == 0.0f) ? avail_width : width;

    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    // Draw Label (Left aligned)

    // Align text vertically to center
    ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + style.FramePadding.y));
    ImGui::TextUnformatted(label);

    // Interaction Zone (Right aligned)
    // Move cursor to the toggle position for the hit-test button
    const float toggle_x = cursor.x + total_width - toggle_w;
    ImGui::SetCursorScreenPos(ImVec2(toggle_x, cursor.y));

    // Create the invisible button only over the toggle area
    // Use a unique ID based on the label to avoid ID conflicts
    const bool pressed = ImGui::InvisibleButton(id_label, ImVec2(toggle_w, height));
    if (pressed)
        *value = !*value;

    // Animation / Rendering
    // float t = *value ? 1.0f : 0.0f; // No animation state storage for simplicity, instant switch

    // Determine colors based on current ImGui Style
    ImU32 col_bg;
    if (*value)
        col_bg = ImGui::GetColorU32(ImGuiCol_ButtonActive); // Active color (usually blue/green) for ON
    else
        col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);      // Frame color (usually dark gray) for OFF

    const ImU32 col_circle = ImGui::GetColorU32(ImGuiCol_Text);   // Knob is text color (usually white)

    // Draw Capsule (Background)
    draw_list->AddRectFilled(ImVec2(toggle_x, cursor.y),
        ImVec2(toggle_x + toggle_w, cursor.y + height),
        col_bg,
        height * 0.5f);

    // Draw Knob (Circle)
    // Calculate offset: left side or right side inside the capsule
    const float knob_r = radius * 0.85f; // Slightly smaller to fit inside
    const float knob_x = (*value) ? (toggle_x + toggle_w - radius) : (toggle_x + radius);

    draw_list->AddCircleFilled(ImVec2(knob_x, cursor.y + radius), knob_r, col_circle);

    // Advance cursor for the next item (restore layout flow)
    // Because we messed with SetCursorScreenPos, we need to manually simulate ItemSize
    // taking the full width into account.
    ImGui::SetCursorScreenPos(cursor); // Reset to start
    ImGui::Dummy(ImVec2(total_width, height)); // Occupy space

    return pressed;
}

bool BooleanToggleLED(const char* id_label, bool* value, bool draw_frame, bool left_rounded, float width /*= 0.0f*/) noexcept
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(id_label);

    // Layout
    const float height = ImGui::GetFrameHeight();
    const float total_width = (0.0f == width ? ImGui::GetContentRegionAvail().x : width);

    const ImVec2 cursor = window->DC.CursorPos;
    const ImRect total_bb(cursor, ImVec2(cursor.x + total_width, cursor.y + height));

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    // Interaction
    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
    {
        *value = !*value;
        // trigger ImGui::IsItemDeactivatedAfterEdit()
        ImGui::MarkItemEdited(id);
    }

    // Draw Frame (Oval/Capsule)
    // Force rounding to height * 0.5 to make it a perfect pill/oval
    const float rounding = height * 0.5f;

    const ImU32 col_frame_bg = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);

    // Draw background and border manually to ensure perfect Oval shape
    window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, col_frame_bg, rounding, left_rounded ? 0 : ImDrawFlags_RoundCornersRight);

    if (draw_frame)
    {
        // Draw border (ImGuiCol_Border)
        window->DrawList->AddRect(total_bb.Min, total_bb.Max, ImGui::GetColorU32(ImGuiCol_Border), rounding, 0, 1.0f);
    }

    // Draw LED
    // Full height logic requested, minus minimal padding to not overlap border
    const float radius = (height * 0.5f) - 2.0f;

    // Position: Right side, centered vertically
    const ImVec2 center(total_bb.Max.x - (height * 0.5f), total_bb.Min.y + (height * 0.5f));

    const ImU32 col_led = *value ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_TextDisabled);
    window->DrawList->AddCircleFilled(center, radius, col_led);

    // Draw Label
    ImGui::RenderText(ImVec2(cursor.x + style.FramePadding.x /*+ rounding * 0.5f*/, cursor.y + style.FramePadding.y), id_label);

    return pressed;
}

bool UrlButton(const char* id_label, const char* tooltip /*= nullptr*/, ImU32 url_clr_def /*= 0*/, ImU32 url_clr_hover /*= 0*/, bool underline /*= true*/) noexcept
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if (0 == url_clr_def)
        url_clr_def = g::pOptsManager->color_load(AppColor::UrlTextDefault);
    if (0 == url_clr_hover)
        url_clr_hover = g::pOptsManager->color_load(AppColor::UrlTextHovered);

    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 label_size = ImGui::CalcTextSize(id_label);

    const bool clicked = ImGui::InvisibleButton(id_label, label_size);
    const bool hovered = ImGui::IsItemHovered();

    const ImU32 text_color = hovered ? url_clr_hover : url_clr_def;
    window->DrawList->AddText(p, text_color, id_label);

    if (underline)
    {
        const float line_y = p.y + label_size.y /*+- 1.0f*/;
        window->DrawList->AddLine(ImVec2(p.x, line_y), ImVec2(p.x + label_size.x, line_y), text_color, 1.0f);
    }

    if (hovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        if (tooltip != nullptr && *tooltip != 0 && ImGui::BeginTooltip())
        {
            ImGui::PushTextWrapPos(300.0f);
            ImGui::TextUnformatted(tooltip);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    return clicked;
}

Toast toast_prepare(const char* text, ImU32 frame_clr, float duration_in_ms, bool at_cursor, bool increase_wnd, const ImVec2* pchild_pos, const ImVec2* pchild_size)
{
    Toast t;
    t.expired = false;
    if (increase_wnd)
        t.message = fmt::format(FMT_STRING("\n     {}     \n\n"), text);
    else
        t.message = text;
    t.duration = duration_in_ms / 1000;
    t.start_time = ImGui::GetTime();
    t.frame_clr = frame_clr;

    t.at_cursor_pos = at_cursor;
    if (at_cursor)
    {
        t.pos = ImGui::GetMousePos();
        t.pos.x += 10;
        t.pos.y -= 20;
    }
    else
    {
        if (pchild_pos != nullptr && pchild_size != nullptr)
        {
            t.pos.x = pchild_pos->x + pchild_size->x * 0.5f;
            t.pos.y = pchild_pos->y + pchild_size->y * 0.5f;
        }
        else
        {
            const ImVec2 child_pos = ImGui::GetWindowPos();
            const ImVec2 child_size = ImGui::GetWindowSize();
            t.pos.x = child_pos.x + child_size.x * 0.5f;
            t.pos.y = child_pos.y + child_size.y * 0.5f;
        }
    }
    return t;
}

void toast_show(Toast& toast) noexcept
{
    if (toast.expired)
        return;

    const double now = ImGui::GetTime();
    const double elapsed = now - toast.start_time;

    if (elapsed < toast.duration)
    {
        constexpr float rounding = 7.0f;

        float alpha = 1.0f;
        if (elapsed > toast.duration * 0.5f)
            alpha = 1.0f - cast::to_float(((elapsed - toast.duration * 0.7f) / (toast.duration * 0.3f)));

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

        if (toast.at_cursor_pos)
            ImGui::SetNextWindowPos(toast.pos);
        else
            ImGui::SetNextWindowPos(toast.pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::BeginTooltip();
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        ImGui::TextUnformatted(toast.message.c_str(), toast.message.c_str() + toast.message.size());

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 rect_min = ImVec2(pos.x + 3.0f, pos.y + 3.0f);
        const ImVec2 rect_max = ImVec2(pos.x + size.x - 3.0f, pos.y + size.y - 3.0f);
        draw_list->AddRect(rect_min, rect_max, toast.frame_clr, rounding, 0, 1.0f);
        ImGui::EndTooltip();

        ImGui::PopStyleVar(2);

        ImGui::PopStyleVar();
    }
    else
        toast.expired = true;
}

void IndeterminateProgressBar(const ImVec2& size_arg) noexcept
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 size = size_arg;
    if (size.x <= 0.0f) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0.0f) size.y = ImGui::GetFrameHeight();

    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);

    const float rounding = ImGui::GetStyle().FrameRounding;

    const ImU32 bg_color = ImGui::GetColorU32(ImGuiCol_FrameBg);
    draw_list->AddRectFilled(p0, p1, bg_color, rounding);

    const float time = (float)ImGui::GetTime();
    constexpr float speed = 1.0f;
    const float strip_width = size.x * 0.25f;

    const float t = (sinf(time * speed) + 1.0f) * 0.5f;
    //const float t = fmodf(time * speed, 1.0f);
    const float strip_x_start = p0.x + (t * (size.x - strip_width));
    const float strip_x_end = strip_x_start + strip_width;

    const ImU32 fg_color = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
    const ImVec2 strip_p0 = ImVec2(strip_x_start, p0.y);
    const ImVec2 strip_p1 = ImVec2(strip_x_end, p1.y);
    draw_list->AddRectFilled(strip_p0, strip_p1, fg_color, rounding);

    ImGui::Dummy(size);
}

//
//// 2. Switch Toggle (Oval Frame, Integrated Track)
//bool ToggleButton(const char* id_label, const char* label_on, const char* label_off, bool* v, float width = 0.0f)
//{
//    ImGuiWindow* window = ImGui::GetCurrentWindow();
//    if (window->SkipItems)
//        return false;
//
//    ImGuiContext& g = *GImGui;
//    const ImGuiStyle& style = g.Style;
//    const ImGuiID id = window->GetID(id_label);
//
//    // 1. Layout
//    float height = ImGui::GetFrameHeight();
//    float total_width = (width == 0.0f) ? ImGui::GetContentRegionAvail().x : width;
//
//    ImVec2 p = window->DC.CursorPos;
//    ImRect total_bb(p, ImVec2(p.x + total_width, p.y + height));
//
//    ImGui::ItemSize(total_bb, style.FramePadding.y);
//    if (!ImGui::ItemAdd(total_bb, id))
//        return false;
//
//    // 2. Interaction
//    bool hovered, held;
//    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
//    if (pressed)
//        *v = !*v;
//
//    // 3. Draw Background & Frame (The "Track" is the Frame)
//    float rounding = height * 0.5f; // Perfect Oval
//
//    ImU32 col_bg;
//    ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
//    ImU32 col_knob = ImGui::GetColorU32(ImGuiCol_Text); // White knob usually
//
//    if (*v)
//    {
//        // ON: The whole capsule is the "Active" color
//        col_bg = ImGui::GetColorU32(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_ButtonActive);
//    }
//    else
//    {
//        // OFF: The whole capsule is the "Frame" color
//        col_bg = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
//        // Dim the knob slightly if needed, or keep white. Standard is white knob on dark bg.
//        col_knob = ImGui::GetColorU32(ImGuiCol_Text);
//    }
//
//    // Draw Capsule Background
//    window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, col_bg, rounding);
//    // Draw Border
//    window->DrawList->AddRect(total_bb.Min, total_bb.Max, col_border, rounding, 0, 1.0f);
//
//    // 4. Draw Knob (Circle)
//    // Radius = half height minus border thickness/padding
//    float radius = (height * 0.5f) - 2.0f;
//
//    // X Position:
//    // If ON: Right side
//    // If OFF: We keep it on the Right side (like an LED toggle) OR move it Left?
//    // Since you asked for a "Switch", usually it moves.
//    // BUT given the layout (Text on Left), the knob usually stays on the Right in list-style toggles.
//    // Let's assume the knob stays on the Right (like iOS list toggles), but the color changes.
//    // If you want it to move physically like a slider, the Text would be in the way.
//    // Assuming "Switch" visual means the knob is always on the Right side of the container.
//
//    ImVec2 knob_center(total_bb.Max.x - (height * 0.5f), total_bb.Min.y + (height * 0.5f));
//
//    // Alternative: If you specifically wanted the knob to slide from left to right inside the capsule,
//    // the text would overlap. Assuming List-View style where control is on the right.
//
//    window->DrawList->AddCircleFilled(knob_center, radius, col_knob);
//
//    // 5. Draw Label
//    const char* label = (*v) ? label_on : label_off;
//    // Add a bit more padding on left because of the heavy rounding
//    ImGui::RenderText(ImVec2(p.x + style.FramePadding.x + rounding * 0.5f, p.y + style.FramePadding.y), label);
//
//    return pressed;
//}
//

//bool ToggleLED2(const char* label, bool* v, float width = 0.0f)
//{
//    ImGuiWindow* window = ImGui::GetCurrentWindow();
//    if (window->SkipItems)
//        return false;
//
//    ImGuiContext& g = *GImGui;
//    const ImGuiStyle& style = g.Style;
//    const ImGuiID id = window->GetID(label);
//
//    // 1. Geometry layout
//    float height = ImGui::GetFrameHeight();
//    float total_width = (width == 0.0f) ? ImGui::GetContentRegionAvail().x : width;
//
//    ImVec2 p = window->DC.CursorPos;
//    ImRect total_bb(p, ImVec2(p.x + total_width, p.y + height));
//
//    // Register item
//    ImGui::ItemSize(total_bb, style.FramePadding.y);
//    if (!ImGui::ItemAdd(total_bb, id))
//        return false;
//
//    // 2. Interaction
//    bool hovered, held;
//    // Making the whole row clickable allows for faster interaction without pixel-hunting
//    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
//
//    if (pressed)
//        *v = !*v;
//
//    // 3. Visuals
//    // LED Geometry
//    float radius = height * 0.30f; // LED size (adjust to taste, usually smaller than text height)
//    float pad_right = style.FramePadding.x; // Padding from right edge
//    ImVec2 center(p.x + total_width - radius - pad_right, p.y + height * 0.5f);
//
//    // Colors
//    // On: Active Color (Green/Blue/etc)
//    // Off: Frame Background (Gray) or TextDisabled
//    ImU32 col_bg;
//    if (*v)
//        col_bg = ImGui::GetColorU32(ImGuiCol_ButtonActive);
//    else
//        col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
//
//    // Hover effect: slight brighten/glow
//    if (hovered)
//        col_bg = ImGui::GetColorU32(*v ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBgHovered);
//
//    // Border color (optional, adds definition to the OFF state)
//    ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
//
//    // 4. Draw
//    // Draw Text (Left)
//    ImVec2 text_pos(p.x + style.FramePadding.x, p.y + style.FramePadding.y);
//    ImGui::RenderText(text_pos, label);
//
//    // Draw LED (Right)
//    window->DrawList->AddCircleFilled(center, radius, col_bg);
//
//    // Optional: Draw a thin border ring so the OFF state isn't invisible against dark backgrounds
//    if (!*v)
//        window->DrawList->AddCircle(center, radius, col_border, 0, 1.0f);
//
//    return pressed;
//}


} // namespace ImGuiEx
