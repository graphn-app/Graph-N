#pragma once
#ifdef _WIN32
#include "ui/helpers/video_control.h"

struct IMFSourceReader;
struct ID3D11Device;

namespace platform
{

class VideoControl_Win32_Gif;

class VideoControl_Win32 final : public global::VideoControl
{
public:
    VideoControl_Win32() noexcept;
    ~VideoControl_Win32() noexcept override;

    void init() override;
    void stop_and_cleanup() override;

protected:
    void cleanup_current() override;

    mngr::VideoInfo get_info(const std::filesystem::path& filepath, uint32_t max_side_len, std::optional<bool>& valid) override;
    int32_t load(const std::filesystem::path& filepath, uint32_t max_side_len) override;

    void play_impl(int32_t video_id) noexcept override;
    void stop_impl(int32_t video_id) noexcept override;
    void seek(int32_t video_id, float pos) noexcept override;

    bool get_is_playing(int32_t video_id) const noexcept override;
    float get_position(int32_t video_id) const noexcept override;

    ImTextureID get_frame_impl(int32_t video_id, float* out_w = nullptr, float* out_h = nullptr) override;

private:
    void decode_thread_loop(mngr::VideoInfo info);
    bool get_thumbnail_check_window_cache(const std::filesystem::path& filename, const std::string& dest_filename, uint32_t max_side_len);

private:
    std::unique_ptr<::IMFSourceReader, std::function<void(::IMFSourceReader*)>> m_reader;
    mngr::VideoInfo m_info;

    bool m_has_video = false;
    std::atomic_bool m_is_playing = false;
    std::atomic<float> m_current_time = 0.0f;
    std::atomic<std::chrono::steady_clock::time_point> m_last_frame_request;

    std::thread m_decode_thread;
    std::stop_source m_stop_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    ImageData m_frame_buffer;
    std::atomic_bool m_frame_ready = false;

    std::atomic_bool m_seek_requested = false;
    std::atomic<float> m_seek_target = 0.0f;

    ImTextureIDPtr m_texture = nullptr;

    bool m_gif_playing = false;
    std::unique_ptr<VideoControl_Win32_Gif> m_gif_player;
};

} // namespace platform

#endif // _WIN32
