
#include "ui/helpers/text_wrapper_ro.h"

namespace ui
{

TextWrapperRO::TextWrapperRO(TextWrapperRO&& other) noexcept
{
    *this = std::move(other);
}

TextWrapperRO& TextWrapperRO::operator=(TextWrapperRO&& other) noexcept
{
    if (this == &other)
        return *this;

    m_font = other.m_font;
    m_raw_text = other.m_raw_text;
    other.m_raw_text = nullptr;
    m_wrapped_text.swap(other.m_wrapped_text);
    m_wrapped_h = other.m_wrapped_h;
    m_calculated_w = other.m_calculated_w;
    return *this;
}

void TextWrapperRO::set_raw_text(const std::string* text)
{
    LogRAssert(text, "Zero ptr", );
    m_raw_text = text;
    m_wrapped_text.reserve(text->size() * 2);
    m_calculated_w = 0.0f;
}

void TextWrapperRO::re_calc(ImVec2 rect)
{
    if (rect.x == m_calculated_w)
        return;

    m_wrapped_text.clear();

    const ImGuiStyle& style = ImGui::GetStyle();
    const float line_height = ImGui::GetTextLineHeight();
    const float min_height = (2 * line_height) + (style.FramePadding.y * 2.0f);
    m_wrapped_h = min_height;

    if (nullptr == m_raw_text || m_raw_text->empty())
        return;

    const auto need_wrap_on_char = [](char c) noexcept
    {
        return (' ' == c || '\t' == c);
    };

    const auto get_next_line_start = [&need_wrap_on_char](const char* text, const char* text_end) noexcept
    {
        while (text < text_end && need_wrap_on_char(*text))
            text++;
        if (*text == '\n')
            text++;
        return text;
    };

    const char* text_begin_const = m_raw_text->c_str();
    const char* line_begin = text_begin_const;
    const char* text_end_const = text_begin_const + m_raw_text->size();

    float line_count = 1.0f;
    while (line_begin < text_end_const)
    {
        const char* line_end = (const char*)memchr(line_begin, '\n', text_end_const - line_begin);
        line_end = m_font->CalcWordWrapPositionA(1.0f, line_begin, line_end ? line_end : text_end_const, rect.x - 20);
        m_wrapped_text += m_raw_text->substr(line_begin - text_begin_const, line_end - line_begin) + "\n";
        line_count++;
        line_begin = get_next_line_start(line_end, text_end_const);
    }

    m_calculated_w = rect.x;

    const float current_height = (line_count * line_height) + (style.FramePadding.y * 2.0f);
    const float max_height = (10 * line_height) + (style.FramePadding.y * 2.0f);
    m_wrapped_h = std::clamp(current_height, min_height, max_height);
}

} // namespace ui
