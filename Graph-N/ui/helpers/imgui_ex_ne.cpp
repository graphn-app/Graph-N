#include "ui/helpers/imgui_ex_ne.h"
#include  "core/workflow/node_instance.h"
#include  "core/workflow/pin_instance.h"
#include "imgui_node_editor.h"

namespace ImGuiExNe
{

namespace ed = ax::NodeEditor;

void Combo(DeferredAction& act, data::NodeInstance* node, data::PinInstance* pin, const char* label, int64_t* current_item, const char* const items[], int64_t items_count) noexcept
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    if (window->SkipItems) [[unlikely]]
        return;

    const ImVec2 pos_before = ImGui::GetCursorScreenPos();

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float arrow_size = ImGui::GetFrameHeight();
    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    const float w = ImGui::CalcItemWidth();
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(bb.Min, bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &bb)) [[unlikely]]
        return;

    // Open on click
    bool hovered = false, held = false;
    if (ImGui::ButtonBehavior(bb, id, &hovered, &held))
    {
        act.active_node = node;
        act.active_pin = pin;

        act.combo.is_active = true;
        act.combo.do_open = true;

        const ImVec2 min_screen = ed::CanvasToScreen(pos_before);
        act.combo.screen_pos = min_screen;
        act.combo.screen_pos.y += ImGui::GetItemRectSize().y;
        act.combo.width = w;

        act.combo.draw_foo = [=, &act](bool do_open) noexcept
        {
            ImGui::SetNextWindowPos(act.combo.screen_pos);
            ImGui::SetNextWindowSizeConstraints(ImVec2(act.combo.width, 0), ImVec2(FLT_MAX, 300));

            if (do_open)
            {
                ImGui::OpenPopup(0x592DCC05);
                act.combo.old_value = act.active_pin->state().value.get_int();
            }
            const bool popup_open = ImGui::BeginPopup(0x592DCC05);
            if (popup_open)
            {
                for (int32_t i = 0; i < items_count; i++)
                {
                    const bool is_selected = (*current_item == i);
                    if (ImGui::Selectable(items[i], is_selected))
                    {
                        *current_item = i;
                        act.combo.is_changed = true;
                        ImGui::CloseCurrentPopup();
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndPopup();
            }
            return popup_open;
        };
    }

    // Render shape
    const ImU32 frame_col = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
    ImGui::RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(bb.Min, ImVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, ImDrawFlags_RoundCornersLeft);

    const ImU32 bg_col = ImGui::GetColorU32((act.combo.is_active || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
    if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
        ImGui::RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);

    ImGui::RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    // Render preview and label
    if (items != nullptr)
        ImGui::RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), items[*current_item], nullptr, nullptr);
    if (label_size.x > 0)
        ImGui::RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), label);
}

void RenderDeferredCombo(DeferredAction& act)
{
    if (act.combo.is_active && act.combo.draw_foo)
    {
        act.combo.is_active = act.combo.draw_foo(act.combo.do_open);
        act.combo.do_open = false;
        if (!act.combo.is_active)
        {
            act.active_node = nullptr;
            act.active_pin = nullptr;
        }
    }
}

bool InputTextWithHint(DeferredAction& act, data::NodeInstance* node, data::PinInstance* pin, const char* label, const char* hint, std::string* str) noexcept
{
    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) [[unlikely]]
        return false;

    const float height = (ImGui::GetTextLineHeight() * 4) + (style.FramePadding.y * 2.0f);
    const float width = ImGui::CalcItemWidth();
    const ImVec2 size(0 == width ? 1.0f : width, height);

    ImGui::PushID(label); 
    const bool pressed = ImGui::InvisibleButton(label, size);
    ImGui::PopID();
    const bool hovered = ImGui::IsItemHovered();

    const ImVec2 p_min = ImGui::GetItemRectMin();
    const ImVec2 p_max = ImGui::GetItemRectMax();

    if (pressed)
    {
        act.active_pin = pin;
        act.active_node = node;
        act.ml_input.is_active = true;
        act.ml_input.buffer = str;
        act.ml_input.hint = hint;
    }

    const ImU32 bg_col = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    const ImU32 border_col = ImGui::GetColorU32(ImGuiCol_Border);

    window->DrawList->AddRectFilled(p_min, p_max, bg_col, style.FrameRounding);
    if (style.FrameBorderSize > 0.0f)
        window->DrawList->AddRect(p_min, p_max, border_col, style.FrameRounding, 0, style.FrameBorderSize);

    ImGui::PushClipRect(p_min, p_max, true); 

    const ImVec2 text_pos = p_min + style.FramePadding;

    if (str != nullptr && !str->empty())
    {
        window->DrawList->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), str->c_str());
    }
    else if (hint != nullptr && *hint != 0)
    {
        window->DrawList->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), hint);
    }

    ImGui::PopClipRect();

    return pressed;
}

} // namespace ImGuiExNe
