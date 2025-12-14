
#pragma once

enum class OptPane
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
		bool show_inner_child = false;
		OptPane left_selected_item = OptPane::_none;
	}
	pane;

	struct Comfy
	{
		std::string host;
		uint16_t port = 0;
		size_t conn_time = 0;
		size_t read_time = 0;
	}
	comfy;
};
