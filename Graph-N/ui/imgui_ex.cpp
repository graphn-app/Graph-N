
#include "ui/imgui_ex.h"

namespace ImGui
{

void PushGID(gid id)
{
	return PushID(std::to_string(id).c_str());
}

void PushLID(lid id)
{
	return PushID(cast::to_i32(id));
}

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

void HelpMarkerOnLastControl(const char* desc)
{
	if (IsItemHovered(ImGuiHoveredFlags_DelayShort) && BeginTooltip())
	{
		PushTextWrapPos(GetFontSize() * 35.0f);
		TextUnformatted(desc);
		PopTextWrapPos();
		EndTooltip();
	}
}

void HelpMarker(const char* desc, const char* text /*= "[?]"*/, bool same_line /*= true*/)
{
	if (same_line)
		SameLine();
	TextDisabled(text);
	HelpMarkerOnLastControl(desc);
}

bool CheckMultiselectControls()
{
	const int mouse_button = (ImGuiPopupFlags_MouseButtonRight & ImGuiPopupFlags_MouseButtonMask_);
	const bool r_click = (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
	return r_click || GetIO().KeyCtrl || GetIO().KeyShift;
}

int DatePicker(const char* label, tt::dt& date, uint16_t min_year, uint16_t max_year, bool& year_enabled, bool allow_disable_year /*= true*/)
{
	if (!date.is_valid())
		return -1;

	constexpr auto picker_id_shift = GLOBAL_ID_DATE_PICKER_ID_FIRST;
	constexpr auto picker_id_init_day = picker_id_shift + 1;
	constexpr auto picker_id_init_month = picker_id_shift + 2;
	constexpr auto picker_id_init_year = picker_id_shift + 3;

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();

	if (window->SkipItems)
		return 0;

	ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const std::string date_formatted = date.as_string(true, false, true);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const float width_adder = 4.f * style.ItemSpacing.x;
	const float year_width = CalcTextSize("9999").x;
	const float width_required_by_calendar = CalcTextSize(tt::dt::month_name(byte(9))).x + year_width + 50.f + width_adder; // month 9 has longest name
	const float width_required_by_textbox = label_size.x + CalcTextSize(date_formatted.c_str()).x;// + style.FramePadding.x*2.0f;
	const float result_width = width_required_by_calendar > width_required_by_textbox ? width_required_by_calendar : width_required_by_textbox;//CalcItemWidth();

	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(result_width, label_size.y) + style.FramePadding * 2.0f);
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(style.ItemInnerSpacing.x + label_size.x, 0));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return 0;

	const bool hovered = ItemHoverable(frame_bb, id, ImGuiItemFlags_None);
	const float arrow_size = (g.FontSize + style.FramePadding.x * 2.0f);

	const ImRect value_bb(frame_bb.Min, frame_bb.Max - ImVec2(arrow_size, 0.0f));
	RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
	RenderFrame(ImVec2(frame_bb.Max.x - arrow_size, frame_bb.Min.y), frame_bb.Max, GetColorU32(ImGuiCol_Button), true, style.FrameRounding);	// FIXME-ROUNDING
	RenderArrow(window->DrawList, ImVec2(frame_bb.Max.x - arrow_size, frame_bb.Min.y) + style.FramePadding, GetColorU32(ImGuiCol_Text), ImGuiDir_Down);

	RenderTextClipped(frame_bb.Min + style.FramePadding, value_bb.Max, date_formatted.c_str(), NULL, NULL);

	if (label_size.x > 0)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	ImGuiStorage* storage = ImGui::GetStateStorage();
	const bool is_open_stored = storage->GetBool(id, false);
	bool is_open = is_open_stored;

	if (hovered && g.IO.MouseClicked[0])
		is_open = !is_open;

	int ret_val = 0;

	PushID(id);
	if (is_open)
	{
		int init_day = storage->GetInt(picker_id_init_day, 0);
		int init_month = storage->GetInt(picker_id_init_month, 0);
		int init_year = storage->GetInt(picker_id_init_year, 0);
		if (0 == init_day)
		{
			storage->SetInt(picker_id_init_day, date.day());
			storage->SetInt(picker_id_init_month, date.month());
			storage->SetInt(picker_id_init_year, date.year());
		}

		int week_num = storage->GetInt(picker_id_shift, 0);

		const int height_in_items = 1 + 1 + 1 + (0 == week_num ? 4 : week_num) + 1;
		const ImVec2 backup_pos = GetCursorPos();
		const float popup_off_x = 0.0f;
		const float popup_height = (label_size.y + 2 * style.ItemSpacing.y) * height_in_items + (style.FramePadding.y * 3);
		const ImRect popup_rect(ImVec2(frame_bb.Min.x + popup_off_x + GetScrollX(), frame_bb.Max.y + GetScrollY()), ImVec2(frame_bb.Max.x + popup_off_x + GetScrollX(), frame_bb.Max.y + popup_height + GetScrollY()));
		SetCursorPos(popup_rect.Min - window->Pos);

		constexpr ImGuiWindowFlags flags = /*ImGuiWindowFlags_ComboBox |*/ ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;// | ImGuiWindowFlags_Modal;    // Adding ImGuiWindowFlags_Modal seems to work too...
		PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
		PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_PopupBg]);
		BeginChild("#DatePickerComboBox", popup_rect.GetSize(), true, flags);
		Spacing(2);

		const float year_part_width = year_width + width_adder * 0.5f;
		const float old_window_rounding = style.WindowRounding;
		style.WindowRounding = 0;

		Text(tt::dt::month_name(date.month()));
		if (IsItemClicked(ImGuiMouseButton_Left))
			OpenPopup("##DatePickerMonthPicker");
		if (BeginPopup("##DatePickerMonthPicker"))
		{
			for (byte i = 0; i < 12; i++)
			{
				if (MenuItem(tt::dt::month_name(byte(i + 1))))
				{
					date = tt::dt::from_date(date.year(), i + 1, date.day());
				}
			}
			EndPopup();
		}

		if (allow_disable_year)
		{
			SameLine(GetWindowWidth() - year_part_width - GetStyle().WindowPadding.x - GetStyle().ItemSpacing.x * 2);
			if (Checkbox("##DatePickerYearPickerCheck", &year_enabled))
			{
				date = tt::dt::from_date((year_enabled ? tt::dt::current_year() : 0), date.month(), date.day());
			}
		}

		SameLine();
		if (year_enabled)
		{
			Text("%d", date.year());
			if (IsItemClicked(ImGuiMouseButton_Left))
				OpenPopup("##DatePickerYearPicker");
			if (BeginPopup("##DatePickerYearPicker"))
			{
				for (uint16_t i = min_year; i <= max_year; i++)
				{
					if (MenuItem(std::to_string(i).c_str()))
					{
						date = tt::dt::from_date(i, date.month(), date.day());
					}
				}
				EndPopup();
			}
		}
		else
		{
			Text("none");
		}

		Spacing();

		Separator();
		PushStyleColor(ImGuiCol_Text, IM_COL32(150, 0, 0, 255));
		Text(" %s", tt::dt::week_day_name(0, 1));
		PopStyleColor();
		const float spacing = GetWindowWidth() / 7;

		for (byte week_day = 1; week_day < 7; week_day++)
		{
			SameLine(spacing * week_day);
			Text(" %s", tt::dt::week_day_name(week_day, 1));
		}
		Separator();
		Spacing();

		const byte num_of_days_in_month = tt::dt::num_of_days_in_month(date.year(), date.month());
		const byte first_day_week = tt::dt::day_of_week(date.year(), date.month(), 1);
		bool started = false;

		PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_HeaderHovered]);
		PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_HeaderActive]);
		const ImVec4 transparent(1, 1, 1, 0);
		ImGui::PushStyleColor(ImGuiCol_Button, transparent);

		const auto draw_day = [&date, &ret_val](byte& day, byte week, bool mark_day)
		{
			if (mark_day)
				PushStyleColor(ImGuiCol_Text, IM_COL32(0, 150, 150, 255));
			else if (0 == week)
				PushStyleColor(ImGuiCol_Text, IM_COL32(150, 0, 0, 255));

			PushID(GLOBAL_ID_DATE_PICKER_ID_NEXT + day);
			if (SmallButton(std::to_string(day).c_str()))
			{
				ClearActiveID();
				date = tt::dt::from_date(date.year(), date.month(), day);
				ret_val = 1;
			}
			PopID();

			if (mark_day || 0 == week)
				PopStyleColor();

			day++;
		};

		week_num = 0;
		for (byte day = 1; day <= num_of_days_in_month; week_num++)
		{
			for (byte week = 0; week < 7 && day <= num_of_days_in_month; week++)
			{
				if (week != 0)
					SameLine(spacing * week);

				if (started)
				{
					draw_day(day, week, (init_day == day && init_month == date.month() && init_year == date.year()));
				}
				else
				{
					if (first_day_week > week)
						Text("  ");
					else
					{
						started = true;
						draw_day(day, week, (init_day == day && init_month == date.month() && init_year == date.year()));
					}
				}
			}
		}

		storage->SetInt(picker_id_shift, --week_num);

		style.WindowRounding = old_window_rounding;
		PopStyleColor(3);

		const float wnd_width = GetWindowSize().x;
		const ImVec2 btn_size(wnd_width / 4 + 3, 0);

		Spacing(2);
		SetCursorPosX(wnd_width / 4 - btn_size.x / 2);
		if (Button("Today##DatePickerToday", btn_size))
		{
			date = tt::dt::from_current_date();
			if (!year_enabled)
				date = tt::dt::from_date(0, date.month(), date.day());
			ret_val = 1;
		}
		SameLine();
		SetCursorPosX(wnd_width / 4 * 3 - btn_size.x / 2);
		if (Button("Cancel##DatePickerCancel", btn_size))
			ret_val = -1;

		EndChild();
		PopStyleColor();
		PopStyleVar();
		SetCursorPos(backup_pos);
	}

	PopID();

	if (ret_val != 0)
	{
		storage->SetInt(picker_id_init_day, 0);
		storage->SetInt(picker_id_init_month, 0);
		storage->SetInt(picker_id_init_year, 0);
		is_open = false;
	}

	if (is_open != is_open_stored)
		storage->SetBool(id, is_open);

	return ret_val;
}

int DatePickerRaw(const char* label, tt::dt& date, uint16_t min_year, uint16_t max_year, bool& year_enabled, bool allow_disable_year /*= true*/)
{
	if (!date.is_valid())
		return -1;

	constexpr auto picker_id_shift = GLOBAL_ID_DATE_PICKER_ID_FIRST;
	constexpr auto picker_id_init_day = picker_id_shift + 1;
	constexpr auto picker_id_init_month = picker_id_shift + 2;
	constexpr auto picker_id_init_year = picker_id_shift + 3;

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();

	if (window->SkipItems)
		return 0;

	ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const std::string date_formatted = date.as_string(true, false, true);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const float width_adder = 4.f * style.ItemSpacing.x;
	const float year_width = CalcTextSize("9999").x;
	const float width_required_by_calendar = CalcTextSize(tt::dt::month_name(byte(9))).x + year_width + 50.f + width_adder; // month 9 has longest name
	const float width_required_by_textbox = label_size.x + CalcTextSize(date_formatted.c_str()).x;// + style.FramePadding.x*2.0f;
	const float result_width = width_required_by_calendar > width_required_by_textbox ? width_required_by_calendar : width_required_by_textbox;//CalcItemWidth();

	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(result_width, /*label_size.y*/0) + style.FramePadding * 2.0f);
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(style.ItemInnerSpacing.x + /*label_size.x*/0, 0));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return 0;

	const bool hovered = ItemHoverable(frame_bb, id, ImGuiItemFlags_None);
	const float arrow_size = (g.FontSize + style.FramePadding.x * 2.0f);
	const ImRect value_bb(frame_bb.Min, frame_bb.Max - ImVec2(arrow_size, 0.0f));

	PushID(id);

	int ret_val = 0;
	ImGuiStorage* storage = GetStateStorage();
	int week_num = storage->GetInt(picker_id_shift, 0);

	int init_day = storage->GetInt(picker_id_init_day, 0);
	int init_month = storage->GetInt(picker_id_init_month, 0);
	int init_year = storage->GetInt(picker_id_init_year, 0);
	if (0 == init_day)
	{
		storage->SetInt(picker_id_init_day, date.day());
		storage->SetInt(picker_id_init_month, date.month());
		storage->SetInt(picker_id_init_year, date.year());
	}

	const int height_in_items = 1 + 1 + 1 + (0 == week_num ? 4 : week_num) + 1;
	const ImVec2 backup_pos = GetCursorPos();
	const float popup_off_x = 0.0f;
	const float popup_height = (label_size.y + 2 * style.ItemSpacing.y) * height_in_items + (style.FramePadding.y * 3);
	const ImRect popup_rect(ImVec2(frame_bb.Min.x + popup_off_x + GetScrollX(), frame_bb.Max.y + GetScrollY()), ImVec2(frame_bb.Max.x + popup_off_x + GetScrollX(), frame_bb.Max.y + popup_height + GetScrollY()));
	SetCursorPos(popup_rect.Min - window->Pos);

	constexpr ImGuiWindowFlags flags = /*ImGuiWindowFlags_ComboBox |*/ ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;// | ImGuiWindowFlags_Modal;    // Adding ImGuiWindowFlags_Modal seems to work too...
	//PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
	PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_PopupBg]);
	BeginChild("#DatePickerComboBox", popup_rect.GetSize(), false, flags);
	Spacing(2);

	const float year_part_width = year_width + width_adder * 0.5f;
	const float old_window_rounding = style.WindowRounding;
	style.WindowRounding = 0;

	Text(tt::dt::month_name(date.month()));
	if (IsItemClicked(ImGuiMouseButton_Left))
		OpenPopup("##DatePickerMonthPicker");
	if (BeginPopup("##DatePickerMonthPicker"))
	{
		for (byte i = 0; i < 12; i++)
		{
			if (MenuItem(tt::dt::month_name(byte(i + 1))))
			{
				date = tt::dt::from_date(date.year(), i + 1, 1);
			}
		}
		EndPopup();
	}

	if (allow_disable_year)
	{
		SameLine(GetWindowWidth() - year_part_width - GetStyle().WindowPadding.x - GetStyle().ItemSpacing.x * 2);
		if (Checkbox("##DatePickerYearPickerCheck", &year_enabled))
		{
			date = tt::dt::from_date((year_enabled ? tt::dt::current_year() : 0), date.month(), 1);
		}
	}

	SameLine();
	if (year_enabled)
	{
		Text("%d", date.year());
		if (IsItemClicked(ImGuiMouseButton_Left))
			OpenPopup("##DatePickerYearPicker");
		if (BeginPopup("##DatePickerYearPicker"))
		{
			for (uint16_t i = min_year; i <= max_year; i++)
			{
				if (MenuItem(std::to_string(i).c_str()))
				{
					date = tt::dt::from_date(i, date.month(), 1);
				}
			}
			EndPopup();
		}
	}
	else
	{
		Text("none");
	}

	Spacing();

	Separator();
	PushStyleColor(ImGuiCol_Text, IM_COL32(150, 0, 0, 255));
	Text(" %s", tt::dt::week_day_name(0, 1));
	PopStyleColor();
	const float spacing = GetWindowWidth() / 7;

	for (byte week_day = 1; week_day < 7; week_day++)
	{
		SameLine(spacing * week_day);
		Text(" %s", tt::dt::week_day_name(week_day, 1));
	}
	Separator();
	Spacing();

	const byte num_of_days_in_month = tt::dt::num_of_days_in_month(date.year(), date.month());
	const byte first_day_week = tt::dt::day_of_week(date.year(), date.month(), 1);
	bool started = false;

	PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_HeaderHovered]);
	PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_HeaderActive]);
	const ImVec4 transparent(1, 1, 1, 0);
	ImGui::PushStyleColor(ImGuiCol_Button, transparent);

	const auto draw_day = [&date, &ret_val](byte& day, byte week, bool mark_day)
	{
		if (mark_day)
			PushStyleColor(ImGuiCol_Text, IM_COL32(0, 150, 150, 255));
		else if (0 == week)
			PushStyleColor(ImGuiCol_Text, IM_COL32(150, 0, 0, 255));

		PushID(GLOBAL_ID_DATE_PICKER_ID_NEXT + day);
		if (SmallButton(std::to_string(day).c_str()))
		{
			ClearActiveID();
			date = tt::dt::from_date(date.year(), date.month(), day);
			ret_val = 1;
		}
		PopID();

		if (mark_day || 0 == week)
			PopStyleColor();

		day++;
	};

	week_num = 0;
	for (byte day = 1; day <= num_of_days_in_month; week_num++)
	{
		for (byte week = 0; week < 7 && day <= num_of_days_in_month; week++)
		{
			if (week != 0)
				SameLine(spacing * week);

			if (started)
			{
				draw_day(day, week, (init_day == day && init_month == date.month() && init_year == date.year()));
			}
			else
			{
				if (first_day_week > week)
					Text("  ");
				else
				{
					started = true;
					draw_day(day, week, (init_day == day && init_month == date.month() && init_year == date.year()));
				}
			}
		}
	}

	storage->SetInt(picker_id_shift, --week_num);

	style.WindowRounding = old_window_rounding;
	PopStyleColor(3);

	const float wnd_width = GetWindowSize().x;
	const ImVec2 btn_size(wnd_width / 4 + 3, 0);

	Spacing(2);
	SetCursorPosX(wnd_width / 4 - btn_size.x / 2);
	if (Button("Today##DatePickerToday", btn_size))
	{
		date = tt::dt::from_current_date();
		if (!year_enabled)
			date = tt::dt::from_date(0, date.month(), date.day());
		ret_val = 1;
	}
	SameLine();
	SetCursorPosX(wnd_width / 4 * 3 - btn_size.x / 2);
	if (Button("Cancel##DatePickerCancel", btn_size))
		ret_val = -1;

	EndChild();
	PopStyleColor();
	//PopStyleVar();
	SetCursorPos(backup_pos);

	if (ret_val != 0)
	{
		storage->SetInt(picker_id_init_day, 0);
		storage->SetInt(picker_id_init_month, 0);
		storage->SetInt(picker_id_init_year, 0);
	}
	
	PopID();

	return ret_val;
}

bool DatePickerText(const std::string& label, 
	const tools::time::dt& date, tools::time::dt& buffer,
	uint16_t min_year, uint16_t max_year,
	int month_text_length /*= 3*/)
{
	bool changed = false;

	const std::string r_context_label = label + "RMenu";
	const std::string l_context_label = label + "LMenu";
	const std::string picker_label = label + "DatePicker";

	const std::string dt_text = tools::str::format("%s  %s,  %s",
		(0 == date.month() ? "---" : date.month_name(month_text_length)),
		(0 == date.day() ? "---" : std::to_string(date.day()).c_str()),
		(0 == date.year() ? "---" : std::to_string(date.year()).c_str())
	);
	ImGui::Text(dt_text.c_str());
	if (ImGui::BeginPopupContextItem(l_context_label.c_str(), ImGuiPopupFlags_MouseButtonLeft) ||
		ImGui::BeginPopupContextItem(r_context_label.c_str(), ImGuiPopupFlags_MouseButtonRight))
	{
		if (!buffer.is_valid())
		{
			if (date.is_valid())
				buffer = date;
			else
				buffer = tt::dt::from_current_date();
		}

		bool year_enabled = buffer.year() > 0;
		const int ret = ImGui::DatePickerRaw(picker_label.c_str(), buffer, min_year, max_year, year_enabled, true);
		if (ret != 0)
		{
			if (ret > 0)
				changed = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	return changed;
}

void Label(const char* label, float x_offset, const char* text)
{
	Text(label);
	SameLine();
	SetCursorPosX(x_offset);
	Text(text);
}

void Label(const char* label, float x_offset, std::string_view text)
{
	Label(label, x_offset, text.data());
}

void Label(const char* label, float x_offset, const std::string& text)
{
	Label(label, x_offset, std::string_view(text));
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

} // namespace ImGui
