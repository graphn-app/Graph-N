
#pragma once

enum class PaneType
{
	_none,

	comfyui,

	_first = comfyui,
	_last = comfyui,
	_zero = _none
};

struct OptsData
{
	const uint32_t separator_clr = IM_COL32(0, 180, 180, 255);
	const float input_offset_x = 150;

	struct Pane
	{
		float height = 0;
		float width = 0;
		bool show_inner_child = false;
		PaneType left_selected_item = PaneType::_none;
	}
	pane;

	struct Comfy
	{
		std::string address;
		uint16_t port = 0;
	}
	comfy;
};
