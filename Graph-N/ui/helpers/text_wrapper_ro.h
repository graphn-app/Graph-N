
#pragma once

namespace ui
{

class TextWrapperRO final
{
public:
    TextWrapperRO() noexcept = default;
    ~TextWrapperRO() noexcept = default;

    TextWrapperRO(const TextWrapperRO&) = delete;
    TextWrapperRO& operator=(const TextWrapperRO&) = delete;

    TextWrapperRO(TextWrapperRO&&) noexcept;
    TextWrapperRO& operator=(TextWrapperRO&&) noexcept;

    void set_raw_text(const std::string* text);
    void set_font(ImFont* font) noexcept { m_font = font; }

    void re_calc(ImVec2 rect);

    const char* get_raw_text() const noexcept { return (nullptr == m_raw_text ? nullptr : m_raw_text->c_str()); }
    std::string* get_wrapped_text() noexcept { return &m_wrapped_text; }
    size_t get_wrapped_size() const noexcept { return m_wrapped_text.size(); }
    float get_wrapped_height() const noexcept { return m_wrapped_h; }

private:
    ImFont* m_font = nullptr;
    const std::string* m_raw_text = nullptr;
    std::string m_wrapped_text;
    float m_wrapped_h = 0.0f;
    float m_calculated_w = 0.0f;
};

} // namespace ui
