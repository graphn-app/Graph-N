
#include "ui/imgui_ex.h"

namespace ImGuiEx
{

void Spacing(size_t count)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    for (size_t i = 0; i < count; i++)
    {
        ImGui::ItemSize(ImVec2(0, 0));
    }
}

void Separator(const char* text)
{
    ImGui::Spacing();
    ImGui::SeparatorText(text);
}

void Separator(uint32_t color, const char* text /*= ""*/)
{
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::SeparatorText(text);
    ImGui::PopStyleColor();
}

void PushDlgTitleStyles(const uint32_t color)
{
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, color);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, color);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, color);
}

void PopDlgTitleStyles()
{
    ImGui::PopStyleColor(3);
}

void HelpMarkerOnLastControl(const char* tooltip)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(tooltip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void HelpMarker(const char* tooltip, const char* help_mark, bool same_line /*= true*/)
{
    if (same_line)
        ImGui::SameLine();

    const ImGuiContext& g = *GImGui;
    ImGui::PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
    ImGui::TextUnformatted(help_mark);
    ImGui::PopStyleColor();

    HelpMarkerOnLastControl(tooltip);
}

bool CheckMultiselectControls()
{
    const int mouse_button = (ImGuiPopupFlags_MouseButtonRight & ImGuiPopupFlags_MouseButtonMask_);
    const bool r_click = (ImGui::IsMouseReleased(mouse_button) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
    return r_click || ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift;
}

void Label(const char* label, float x_offset, const char* text)
{
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(x_offset);
    ImGui::TextUnformatted(text);
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

bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputTextMultiline(label, (char*)str->c_str(), str->capacity() + 1, size, flags, InputTextCallback, &cb_user_data);
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

template <typename TYPE>
bool InputTypeSmart(const char* label_id,
    TYPE* value, TYPE step, TYPE min, TYPE max, ImGuiDataType_ type,
    const std::string& format_named, const std::string& format_unnamed)
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
    const std::string& format_named, const std::string& format_unnamed)
{
    return InputTypeSmart(label_id, value, step, min, max, ImGuiDataType_S64, format_named, format_unnamed);
}

bool InputUIntSmart(
    const char* label_id,
    uint64_t* value, uint64_t step, uint64_t min, uint64_t max,
    const std::string& format_named, const std::string& format_unnamed)
{
    return InputTypeSmart(label_id, value, step, min, max, ImGuiDataType_U64, format_named, format_unnamed);
}

bool InputDoubleSmart(
    const char* label_id, 
    double* value, double step, double min, double max,
    const std::string& format_named, const std::string& format_unnamed)
{
    return InputTypeSmart(label_id, value, step, min, max, ImGuiDataType_Double, format_named, format_unnamed);
}

template <typename TYPE>
bool DragTypeSmart(const char* label, TYPE* value, TYPE min, TYPE max, ImGuiDataType_ type, const char* format)
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

bool DragDoubleSmart(const char* label, double* value, double min, double max, const char* format)
{
    return DragTypeSmart(label, value, min, max, ImGuiDataType_Double, format);
}

bool DragIntSmart(const char* label, int64_t* value, int64_t min, int64_t max, const char* format)
{
    return DragTypeSmart(label, value, min, max, ImGuiDataType_S64, format);
}

bool DragUIntSmart(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format)
{
    return DragTypeSmart(label, value, min, max, ImGuiDataType_U64, format);
}

template <typename TYPE>
bool SliderTypeSmart(const char* label, TYPE* value, TYPE min, TYPE max, ImGuiDataType_ type, const char* format)
{
    if (ImGui::SliderScalar(label, type, value, &min, &max, format))
    {
        if (*value < min) *value = min;
        if (*value > max) *value = max;
        return true;
    }
    return false;
}

bool SliderDoubleSmart(const char* label, double* value, double min, double max, const char* format)
{
    return SliderTypeSmart(label, value, min, max, ImGuiDataType_Double, format);
}

bool SliderIntSmart(const char* label, int64_t* value, int64_t min, int64_t max, const char* format)
{
    return SliderTypeSmart(label, value, min, max, ImGuiDataType_S64, format);
}

bool SliderUIntSmart(const char* label, uint64_t* value, uint64_t min, uint64_t max, const char* format)
{
    return SliderTypeSmart(label, value, min, max, ImGuiDataType_U64, format);
}

bool BooleanButtons1(const char* id_label_on, const char* id_label_off, bool* value, ImVec2 size_arg /*= ImVec2(0, 0)*/)
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

bool BooleanButtons2(const char* id_label, const char* label, bool* value)
{
    uint8_t u8val = *value, min = 0, max = 1;
    if (ImGui::SliderScalar(id_label, ImGuiDataType_U8, &u8val, &min, &max, label, ImGuiSliderFlags_NoInput))
    {
        *value = (1 == u8val);
        return true;
    }
    return false;
}

bool BooleanSwitch(const char* id_label, const char* label, bool* value, float width /*= 0.0f*/)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiStyle& style = ImGui::GetStyle();

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

bool BooleanToggleLED(const char* id_label, bool* value, float width /*= 0.0f*/)
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
        *value = !*value;

    // Draw Frame (Oval/Capsule)
    // Force rounding to height * 0.5 to make it a perfect pill/oval
    const float rounding = height * 0.5f;

    const ImU32 col_frame_bg = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);

    // Draw background and border manually to ensure perfect Oval shape
    window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, col_frame_bg, rounding);

    // Draw border (ImGuiCol_Border)
    window->DrawList->AddRect(total_bb.Min, total_bb.Max, ImGui::GetColorU32(ImGuiCol_Border), rounding, 0, 1.0f);

    // Draw LED
    // Full height logic requested, minus minimal padding to not overlap border
    const float radius = (height * 0.5f) - 2.0f;

    // Position: Right side, centered vertically
    const ImVec2 center(total_bb.Max.x - (height * 0.5f), total_bb.Min.y + (height * 0.5f));

    const ImU32 col_led = *value ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_TextDisabled);
    window->DrawList->AddCircleFilled(center, radius, col_led);

    // Draw Label
    ImGui::RenderText(ImVec2(cursor.x + style.FramePadding.x + rounding * 0.5f, cursor.y + style.FramePadding.y), id_label);

    return pressed;
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
