/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#ifdef _WIN32

#include <wrl/client.h>
#include <windows.h>
#include <wincodec.h>
#include <propvarutil.h>
#include "stb_image_write.h"

using Microsoft::WRL::ComPtr;

namespace platform
{

class VideoControl_Win32_Gif final
{
public:
    VideoControl_Win32_Gif() noexcept;
    ~VideoControl_Win32_Gif() noexcept;

    NO_COPY_MOVE(VideoControl_Win32_Gif);

    bool path_is_gif(const std::filesystem::path& filepath) const;

    bool init() noexcept;
    void stop_and_cleanup();

    mngr::VideoInfo get_info(const std::filesystem::path& filepath, std::optional<bool>& valid);
    bool load(const std::filesystem::path& filepath);

    void play();
    void stop();
    void seek(float pos);

    bool get_is_playing() const noexcept { return m_is_playing; }
    float get_position() const noexcept { return m_current_time; }

    ImTextureID get_frame(float* out_w = nullptr, float* out_h = nullptr);

private:
    void apply_frame(int32_t idx);

private:
    bool m_initialized = false;

    struct FrameMeta final
    {
        float timestamp_sec = 0.0f;
        int32_t delay_ms = 0;
        uint8_t disposal = 0; // 1 = leave, 2 = clear to bg, 3 = restore prev
        uint32_t left = 0, top = 0, width = 0, height = 0;
    };

    mngr::VideoInfo m_info;
    std::vector<FrameMeta> m_frames_meta;

    ComPtr<IWICImagingFactory> m_factory;
    ComPtr<IWICBitmapDecoder> m_decoder;

    std::vector<uint32_t> m_canvas;
    std::vector<uint32_t> m_backup_canvas; // needed for disposal method 3

    bool m_is_playing = false;
    float m_current_time = 0.0f;
    int m_current_frame_idx = -1;

    ImTextureIDPtr m_texture;
};

} // namespace platform

#endif // _WIN32
