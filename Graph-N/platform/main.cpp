
#include "platform/main.h"

namespace platform
{

void MainRoutine::prepare_fonts()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;

    // prepare diapazones
    static constexpr ImWchar ranges_latin_ext [] = { 0x0100, 0x024F, 0 };
    static constexpr ImWchar ranges_emoji     [] = { 0x1F000, 0x1FFFF, 0 };
    static constexpr ImWchar ranges_variations[] = { 0xFE00, 0xFE0F, 0 };
    static constexpr ImWchar ranges_icons     [] = { ICON_MIN_LC, ICON_MAX_LC, 0 };
    static constexpr ImWchar ranges_symbols   [] =
    {
		0x2000, 0x206F, // General Punctuation
		0x20A0, 0x20CF, // Currency Symbols
		0x2100, 0x218F, // Letterlike Symbols (™ и т.д.)
		0x2190, 0x21FF, // Arrows (← ↑ → ↓)
		0x2200, 0x22FF, // Mathematical Operators
		0x2300, 0x23FF, // Misc Technical (⌨ ⌚)
		0x2460, 0x24FF, // Enclosed Alphanumerics (①, ⓐ)
		0x2500, 0x257F, // Box Drawing (║ ╗)
		0x2580, 0x259F, // Block Elements (█ ░)
		0x25A0, 0x25FF, // Geometric Shapes (▶ ▲ ●)
		0x2600, 0x27BF, // Misc Symbols + Dingbats (⚙️, ⚠, ✨, ✂, ✈)
		0,
    };

    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
    builder.AddRanges(ranges_latin_ext);
    builder.AddRanges(ranges_symbols);
    builder.AddRanges(ranges_variations);
    builder.AddRanges(ranges_emoji);
    ImVector<ImWchar> ranges;
    builder.BuildRanges(&ranges);

	// paths
    const auto res_path = fs::get_res_directory_path();
    const auto path_inter_reg = (res_path / "Inter_18pt-Regular.ttf").string();
    const auto path_inter_bold = (res_path / "Inter_18pt-Bold.ttf").string(); // Твой жирный шрифт
    const auto path_emoji = (res_path / "NotoEmoji-Regular.ttf").string();
    const auto path_icons = (res_path / FONT_ICON_FILE_NAME_LC).string();

    // ================= REGULAR FONT =================

    // Base
    config.MergeMode = false;
    m_font_regular = io.Fonts->AddFontFromFileTTF(path_inter_reg.c_str(), 18.0f, &config, ranges.Data);

    // Merge Emoji -> Regular
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 18.0f;
    config.GlyphOffset.y = 1.0f;
    io.Fonts->AddFontFromFileTTF(path_emoji.c_str(), 18.0f, &config, ranges.Data);

    // ================= BOLD FONT =================

    // Base Bold
    config.MergeMode = false;
    config.PixelSnapH = false;
    config.GlyphMinAdvanceX = 0.0f;
    config.GlyphOffset.y = 0.0f;
    m_font_bold = io.Fonts->AddFontFromFileTTF(path_inter_bold.c_str(), 18.0f, &config, ranges.Data);

    // Merge Emoji -> Bold
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 18.0f;
    config.GlyphOffset.y = 1.0f;
    io.Fonts->AddFontFromFileTTF(path_emoji.c_str(), 18.0f, &config, ranges.Data);

    // ================= ICONS FONT =================

    m_font_icons = io.Fonts->AddFontFromFileTTF(path_icons.c_str(), 24.0f, &config, ranges_icons);

    // build
    io.Fonts->Build();

    // ZERO WIDTH HACK
    ImFont* fonts_to_fix[] = { m_font_regular, m_font_bold };
    for (ImFont* font : fonts_to_fix)
    {
        for (auto& glyph : font->Glyphs)
        {
            if (glyph.Codepoint >= 0xFE00 && glyph.Codepoint <= 0xFE0F)
            {
                glyph.AdvanceX = 0.0f;
            }
        }
    }
}

} // namespace platform
