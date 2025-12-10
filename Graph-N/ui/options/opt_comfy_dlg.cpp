
#include "ui/options/opt_comfy_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

void OptComfyDlg::init(OptsData& data)
{
	LogF;

	data.comfy.address = g::pOptsManager->comfyui_get_address();
	data.comfy.port = g::pOptsManager->comfyui_get_port();
}

void OptComfyDlg::show(OptsData& data)
{
	ImGui::Separator(data.separator_clr, "ComfyUI address");

	ImGui::Text("Host:");
	ImGui::SameLine();
	ImGui::SetCursorPosX(data.input_offset_x);
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputText("##ComfyUIHost", &data.comfy.address, ImGuiInputTextFlags_AutoSelectAll))
	{
		g::pOptsManager->comfyui_set_address(data.comfy.address);
	}

	ImGui::Spacing();

	ImGui::Text("Port:");
	ImGui::SameLine();
	ImGui::SetCursorPosX(data.input_offset_x);
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputScalar("##ComfyUIPort", ImGuiDataType_U16, &data.comfy.port, nullptr, nullptr, "%u",
		ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
	{
		g::pOptsManager->comfyui_set_port(data.comfy.port);
	}

	ImGui::Separator(data.separator_clr);

	ImGui::Button("Restart ComfyUI server");
}

}