#pragma once

namespace ImGuiExNe
{

void Combo(void* deffered_id, const char* label, int* current_item, const char* const items[], int items_count);
void RenderDeferredCombo();
void* GetDeferredID();

} // namespace ImGuiExNe
