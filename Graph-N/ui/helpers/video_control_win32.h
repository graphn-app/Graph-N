#pragma once
#ifdef _WIN32
#include "ui/helpers/video_control.h"

struct IMFSourceReader;
struct ID3D11Device;

namespace platform
{

class VideoControl_Win32 final : public global::VideoControl
{
public:
    VideoControl_Win32();
    ~VideoControl_Win32() override;

    void init() override;
    void cleanup() override;

    mngr::VideoInfo get_info(const std::string& filepath, std::optional<bool>& valid) override;

    int32_t load(const std::string& filepath, uint32_t max_side_len) override;
    void play(int32_t video_id) override;
    void stop(int32_t video_id) override;
    void seek(int32_t video_id, float pos) override;

    bool get_is_playing(int32_t video_id) const override;
    float get_position(int32_t video_id) const override;

    ImTextureID get_frame(int32_t video_id, float* out_w = nullptr, float* out_h = nullptr) override;
    std::string get_thumbnail(const std::string& filename, uint32_t max_side_len) override;

private:
    void decode_thread_loop(mngr::VideoInfo info);

private:
    std::unique_ptr<IMFSourceReader, std::function<void(IMFSourceReader*)>> m_reader;
    mngr::VideoInfo m_info;

    bool m_has_video = false;
    std::atomic_bool m_is_playing = false;
    std::atomic<float> m_current_time = 0.0f;
    std::atomic<std::chrono::steady_clock::time_point> m_last_frame_request;

    std::thread m_decode_thread;
    std::atomic_bool m_stop_thread = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    ImageData m_frame_buffer;
    std::atomic_bool m_frame_ready = false;

    std::atomic_bool m_seek_requested = false;
    std::atomic<float> m_seek_target = 0.0f;

    ImTextureIDPtr m_texture = nullptr;
};

} // namespace platform

#endif // _WIN32
