
#include "ui/helpers/imgui_ex_ne.h"
#include "imgui_node_editor.h"

namespace ImGuiExNe
{

namespace ed = ax::NodeEditor;

struct DeferredAction final
{
    void* deffered_id = nullptr;
    bool is_active = false;
    bool do_open = false;
    ImVec2 screen_pos;
    float width = 0.0f;
    std::function<bool(bool)> draw_foo;
};
inline DeferredAction g_combo_deferred_action;

void* GetDeferredID()
{
    return g_combo_deferred_action.deffered_id;
}

void Combo(void* deffered_id, const char* label, int* current_item, const char* const items[], int items_count)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    if (window->SkipItems)
        return;

    const ImVec2 pos_before = ImGui::GetCursorScreenPos();

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float arrow_size = ImGui::GetFrameHeight();
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    const float w = ImGui::CalcItemWidth();
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(bb.Min, bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &bb))
        return;

    // Open on click
    bool hovered = false, held = false, popup_open = false;
    if (ImGui::ButtonBehavior(bb, id, &hovered, &held))
    {
        g_combo_deferred_action.deffered_id = deffered_id;
        g_combo_deferred_action.is_active = true;
        g_combo_deferred_action.do_open = true;

        ImVec2 min_screen = ed::CanvasToScreen(pos_before);
        g_combo_deferred_action.screen_pos = min_screen;
        g_combo_deferred_action.screen_pos.y += ImGui::GetItemRectSize().y;
        g_combo_deferred_action.width = w;

        g_combo_deferred_action.draw_foo = [=, &popup_open](bool do_open)
        {
            ImGui::SetNextWindowPos(g_combo_deferred_action.screen_pos);
            ImGui::SetNextWindowSizeConstraints(ImVec2(g_combo_deferred_action.width, 0), ImVec2(FLT_MAX, 300));

            if (do_open)
                ImGui::OpenPopup("##{592DCC05-159B-44BC-B96C-49FE4643D127}");
            popup_open = ImGui::BeginPopup("##{592DCC05-159B-44BC-B96C-49FE4643D127}");
            if (popup_open)
            {
                for (int i = 0; i < items_count; i++)
                {
                    const bool is_selected = (*current_item == i);
                    if (ImGui::Selectable(items[i], is_selected))
                        *current_item = i;
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

    ImU32 bg_col = ImGui::GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
    if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
        ImGui::RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);

    ImGui::RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    // Render preview and label
    ImGui::RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), items[*current_item], NULL, NULL);
    if (label_size.x > 0)
        ImGui::RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), label);
}

void RenderDeferredCombo()
{
    if (g_combo_deferred_action.is_active && g_combo_deferred_action.draw_foo)
    {
        g_combo_deferred_action.is_active = g_combo_deferred_action.draw_foo(g_combo_deferred_action.do_open);
        g_combo_deferred_action.do_open = false;
        if (!g_combo_deferred_action.is_active)
        {
            g_combo_deferred_action.deffered_id = nullptr;
        }
    }
}

} // namespace ImGuiExNe
