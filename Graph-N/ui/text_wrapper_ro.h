
#pragma once

namespace ui
{

class TextWrapperRO final
{
public:
    NO_COPY_MOVE_CTORS(TextWrapperRO);

    void set_raw_text(const std::string* text);
    void set_font(ImFont* font) { m_font = font; }
    
    void re_calc(ImVec2 rect);

    char* get_wrapped_text() { return m_wrapped_text.data(); }
	size_t get_wrapped_capacity() const { return m_wrapped_text.capacity(); }
    float get_wrapped_height() const { return m_wrapped_h; }

private:
    ImFont* m_font = nullptr;
    const std::string* m_raw_text = nullptr;
    std::string m_wrapped_text;
    float m_wrapped_h = 0.0f;
    float m_calculated_w = 0.0f;
};

} // namespace ui