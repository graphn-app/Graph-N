/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#ifdef __linux__
#include "ui/helpers/video_control.h"

namespace platform
{

class VideoControl_Linux final : public global::VideoControl
{
public:
    VideoControl_Linux() noexcept = default;
    ~VideoControl_Linux() noexcept override;

    void init() override;
    void stop_and_cleanup() override;

    mngr::VideoInfo get_info(const std::filesystem::path& filepath, uint32_t max_side_len, std::optional<bool>& valid) override;
    int32_t load(const std::filesystem::path& filepath, uint32_t max_side_len) override;

    void seek(int32_t video_id, float pos) override;
    bool get_is_playing(int32_t video_id) const noexcept override;
    float get_position(int32_t video_id) const noexcept override;

    void cleanup_current() override;
    void play_impl(int32_t video_id) override;
    void stop_impl(int32_t video_id) override;
    ImTextureID get_frame_impl(int32_t video_id, float* out_w, float* out_h) override;
};

} // namespace platform

#endif // __linux__
