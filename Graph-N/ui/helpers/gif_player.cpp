/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/helpers/gif_player.h"
#include "ui/helpers/textures.h"
#include "stb_image_write.h"

namespace global
{

bool GifPlayer::path_is_gif(const std::filesystem::path& filepath) const
{
    const auto ext = tools::str::to_lower_ascii(filepath.extension().string());
    return ext == ".gif";
}

void GifPlayer::stop_and_cleanup()
{
    m_is_playing = false;
    m_current_time = 0.0f;
    m_current_frame_idx = -1;

    if (m_frames != nullptr)
        stbi_image_free(m_frames);
    if (m_delays != nullptr)
        stbi_image_free(m_delays);

    m_frames = nullptr;
    m_delays = nullptr;

    m_info = {};
    m_timestamps.clear();
    m_texture.reset();
}

mngr::VideoInfo GifPlayer::get_info(const std::filesystem::path& filepath, std::optional<bool>& valid)
{
    valid = false;
    const auto file_buffer = tools::path::read_file(filepath, false);
    if (file_buffer.empty())
        return {};

    int32_t w, h, frames, comp;
    int32_t* delays = nullptr;

    // load GIF just to extract metadata and the first frame
    stbi_uc* data = stbi_load_gif_from_memory(cast::ptrs<stbi_uc>(file_buffer.data()), cast::to_i32(file_buffer.size()), &delays, &w, &h, &frames, &comp, 4);
    if (!data)
        return {};

    mngr::VideoInfo info;
    info.is_gif = true;
    info.width = w;
    info.height = h;

    float accum_time = 0.0f;
    for (int32_t i = 0; i < frames; ++i)
    {
        int32_t delay_ms = delays[i];
        if (delay_ms <= 10) delay_ms = 100;
        accum_time += cast::to_float(delay_ms) / 1000.0f;
    }

    info.duration_sec = accum_time;
    info.fps = frames > 0 ? (cast::to_float(frames) / accum_time) : 0.0f;    

    // write only the first frame (w * h * 4 bytes)
    info.thumbnail = tools::path::path_to_string(fs::get_temp_filepath(".png"));
    if (stbi_write_png(info.thumbnail.c_str(), w, h, 4, data, w * 4) != 0)
    {
        valid = true;
    }

    stbi_image_free(data);
    stbi_image_free(delays);
    return info;
}

bool GifPlayer::load(const std::filesystem::path& filepath)
{
    stop_and_cleanup();
    m_info.is_gif = true;

    const auto file_buffer = tools::path::read_file(filepath, false);
    if (file_buffer.empty())
        return false;

    int32_t frames, comp;
    m_frames = stbi_load_gif_from_memory(cast::ptrs<stbi_uc>(file_buffer.data()), cast::to_i32(file_buffer.size()), &m_delays, &m_info.width, &m_info.height, &frames, &comp, 4);
    if (!m_frames)
        return false;

    // we scale logically if needed, though STB doesn't rescale on load. 
    // real scaling should be done during GPU upload or rendering.

    m_timestamps.reserve(frames);
    float accum_time = 0.0f;
    for (int32_t i = 0; i < frames; ++i)
    {
        m_timestamps.push_back(accum_time);
        int32_t delay_ms = m_delays[i];
        if (delay_ms <= 10) delay_ms = 100;
        accum_time += cast::to_float(delay_ms) / 1000.0f;
    }

    m_info.duration_sec = accum_time;
    m_info.fps = frames > 0 ? (cast::to_float(frames) / accum_time) : 0.0f;

    m_current_time = 0.0f;
    m_is_playing = false;
    return true;
}

void GifPlayer::play() noexcept
{
    if (m_frames != nullptr)
    {
        if (m_current_time >= m_info.duration_sec)
            m_current_time = 0.0f;
        m_is_playing = true;
    }
}

void GifPlayer::stop()
{
    stop_and_cleanup(); // full memory release on stop
}

void GifPlayer::seek(float pos)
{
    if (m_frames != nullptr)
    {
        m_current_time = std::clamp(pos, 0.0f, m_info.duration_sec);
    }
}

ImTextureID GifPlayer::get_frame(float* out_w /*= nullptr*/, float* out_h /*= nullptr*/)
{
    if (nullptr == m_frames)
        return nullptr;

    if (out_w) *out_w = cast::to_float(m_info.width);
    if (out_h) *out_h = cast::to_float(m_info.height);

    if (m_is_playing)
    {
        m_current_time += ImGui::GetIO().DeltaTime;
        if (m_current_time >= m_info.duration_sec)
        {
            m_current_time = fmodf(m_current_time, m_info.duration_sec); 
        }
    }

    int32_t target_frame = 0;
    const int32_t total_frames = cast::to_i32(m_timestamps.size());
    for (int32_t i = total_frames - 1; i >= 0; --i)
    {
        if (m_current_time >= m_timestamps[i])
        {
            target_frame = i;
            break;
        }
    }

    if (target_frame != m_current_frame_idx)
    {
        m_current_frame_idx = target_frame;

        // STB returns raw RGBA, ready for IM_COL32 usage
        const size_t frame_offset = cast::to_szt(target_frame) * m_info.width * m_info.height * 4;
        const uint8_t* pixels = m_frames + frame_offset;
        if (!m_texture) m_texture = g::pTextures->upload_to_gpu(pixels, m_info.width, m_info.height, true);
        else g::pTextures->update_gpu_texture(m_texture, pixels, m_info.width, m_info.height, true);
    }

    return m_texture.get();
}

}
