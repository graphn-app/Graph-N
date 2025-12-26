
#include "platform/main.h"

namespace platform
{

void MainRoutine::prepare_fonts()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;

    // prepare diapazones
    static constexpr ImWchar ranges_latin_ext[3] = { 0x0100, 0x024F, 0 };
    static constexpr ImWchar ranges_emoji[3] = { 0x1F000, 0x1FFFF, 0 };
    static constexpr ImWchar ranges_icons[3] = { ICON_MIN_LC, ICON_MAX_LC, 0 };

    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
    builder.AddRanges(ranges_latin_ext);
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
    config.GlyphMinAdvanceX = 18.0f;
    io.Fonts->AddFontFromFileTTF(path_emoji.c_str(), 18.0f, &config, ranges_emoji);

    // Merge Icons -> Regular
    config.GlyphMinAdvanceX = 13.0f;
    config.GlyphOffset.y = 1.0f;
    io.Fonts->AddFontFromFileTTF(path_icons.c_str(), 17.0f, &config, ranges_icons);

    // ================= BOLD FONT =================

    // Base Bold
    config.MergeMode = false;
    config.GlyphMinAdvanceX = 0.0f;
    config.GlyphOffset.y = 0.0f;
    m_font_bold = io.Fonts->AddFontFromFileTTF(path_inter_bold.c_str(), 18.0f, &config, ranges.Data);

    // Merge Emoji -> Bold
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 18.0f;
    io.Fonts->AddFontFromFileTTF(path_emoji.c_str(), 18.0f, &config, ranges_emoji);

    // Merge Icons -> Bold
    config.GlyphMinAdvanceX = 13.0f;
    config.GlyphOffset.y = 1.0f;
    io.Fonts->AddFontFromFileTTF(path_icons.c_str(), 17.0f, &config, ranges_icons);

    // ================= ICONS FONT =================

    m_font_icons = io.Fonts->AddFontFromFileTTF(path_icons.c_str(), 24.0f, &config, ranges_icons);

    // build
    io.Fonts->Build();
}

} // namespace platform
