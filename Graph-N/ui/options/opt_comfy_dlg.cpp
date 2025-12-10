
#include "ui/options/opt_comfy_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

void OptComfyDlg::init(OptsData& data)
{
	LogF;

	data.comfy.host = g::pOptsManager->comfyui_get_host();
	data.comfy.port = g::pOptsManager->comfyui_get_port();
	data.comfy.conn_time = g::pOptsManager->comfyui_get_connect_timeout();
	data.comfy.read_time = g::pOptsManager->comfyui_get_read_timeout();
}

void OptComfyDlg::show(OptsData& data)
{
	ImGui::Separator(data.separator_clr, "ComfyUI address");

	ImGui::Text("Host:");
	ImGui::SameLine();
	ImGui::SetCursorPosX(data.input_offset_x);
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputText("##ComfyUIHost", &data.comfy.host, ImGuiInputTextFlags_AutoSelectAll))
	{
		g::pOptsManager->comfyui_set_host(data.comfy.host);
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

	ImGui::Separator(data.separator_clr, "ComfyUI server timeouts (in seconds)");

	ImGui::Text("Connection:");
	ImGui::SameLine();
	ImGui::SetCursorPosX(data.input_offset_x);
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputScalar("##ComfyUIConnTime", ImGuiDataType_U64, &data.comfy.conn_time, nullptr, nullptr, "%u",
		ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
	{
		g::pOptsManager->comfyui_set_connect_timeout(data.comfy.conn_time);
	}

	ImGui::Text("Reading:");
	ImGui::SameLine();
	ImGui::SetCursorPosX(data.input_offset_x);
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputScalar("##ComfyUIReadTime", ImGuiDataType_U64, &data.comfy.read_time, nullptr, nullptr, "%u",
		ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
	{
		g::pOptsManager->comfyui_set_read_timeout(data.comfy.read_time);
	}

	ImGui::Separator(data.separator_clr);
	ImGui::Button("Restart ComfyUI server");
}

}