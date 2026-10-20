/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace global
{

class GifPlayer final   // DO NOT USE, THIS WILL FREEZE ON BIG GIF
{
private: 
    GifPlayer() noexcept = default;
    ~GifPlayer() noexcept;

    NO_COPY_MOVE(GifPlayer);

    void stop_and_cleanup();

    bool path_is_gif(const std::filesystem::path& filepath) const;

    mngr::VideoInfo get_info(const std::filesystem::path& filepath, std::optional<bool>& valid);
    bool load(const std::filesystem::path& filepath);

    void play() noexcept;
    void stop();
    void seek(float pos);

    bool get_is_playing() const noexcept { return m_is_playing; }
    float get_position() const noexcept { return m_current_time; }

    ImTextureID get_frame(float* out_w = nullptr, float* out_h = nullptr);

private:
    mngr::VideoInfo m_info;

    stbi_uc* m_frames = nullptr;
    int32_t* m_delays = nullptr;
    std::vector<float> m_timestamps;

    bool m_is_playing = false;
    float m_current_time = 0.0f;
    int m_current_frame_idx = -1;

    ImTextureIDPtr m_texture;
};

} // namespace global
