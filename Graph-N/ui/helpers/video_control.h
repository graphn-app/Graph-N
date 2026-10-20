/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/listeners.h"

namespace global
{

class VideoControl : public listener::Timer
{
public:
    VideoControl() noexcept = default;
    virtual ~VideoControl() noexcept;

    NO_COPY_MOVE(VideoControl);

    virtual void init();
    virtual void stop_and_cleanup();

    virtual mngr::VideoInfo get_info(const std::filesystem::path& filepath, uint32_t max_side_len, std::optional<bool>& valid) = 0;
    virtual int32_t load(const std::filesystem::path& filepath, uint32_t max_side_len) = 0;

    int32_t get_current() const noexcept { return m_video_id; }

    void play(int32_t video_id);
    void stop(int32_t video_id);
    virtual void seek(int32_t video_id, float pos) = 0;

    virtual bool get_is_playing(int32_t video_id) const = 0;
    virtual float get_position(int32_t video_id) const = 0;

    ImTextureID get_frame(int32_t video_id, float* out_w = nullptr, float* out_h = nullptr);

protected:
    virtual void cleanup_current() = 0;
    virtual void play_impl(int32_t video_id) = 0;
    virtual void stop_impl(int32_t video_id) = 0;
    virtual ImTextureID get_frame_impl(int32_t video_id, float* out_w, float* out_h) = 0;
    void on_1s_lazy_timer(int64_t total_seconds_elapsed) override;

protected:
    bool m_initialized = false;

    int32_t m_last_video_id = 0;
    int32_t m_video_id = 0;

private:
    double m_last_requested = 0.0;
};

} // namespace global
