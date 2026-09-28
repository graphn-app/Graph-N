#include "ui/helpers/video_control.h"

namespace global
{

VideoControl::~VideoControl() noexcept
{
    try {
        cleanup_all();
    } catch (...) {}
}

void VideoControl::init_all()
{
    init();
    if (!m_initialized)
        return;

    g::pGraphN->subscribe_timers(this);
    Log("Video engine initialized");
}

void VideoControl::cleanup_all()
{
    g::pGraphN->unsubscribe_timers(this);
    cleanup();
    m_initialized = false;
    Log("Video engine deinitialized");
}

void VideoControl::on_1s_lazy_timer(int64_t /*total_seconds_elapsed*/)
{
    if (m_video_id != 0 && m_last_requested > 0.0 && ImGui::GetTime() - m_last_requested >= PLAYING_VIDEO_LIFETIME)
        stop(m_video_id);
}

void VideoControl::play(int32_t video_id)
{
    LogRAssert(m_video_id == video_id, "Video must be loaded first", );
    play_impl(video_id);
    m_last_requested = ImGui::GetTime();
}

void VideoControl::stop(int32_t video_id)
{
    LogRAssert(m_video_id == video_id, "Video must be loaded first", );
    stop_impl(video_id);
    m_last_requested = 0.0;
}

ImTextureID VideoControl::get_frame(int32_t video_id, float* out_w /*= nullptr*/, float* out_h /*= nullptr*/)
{
    LogRAssert(m_video_id == video_id, "Video must be loaded first", nullptr);
    if (m_video_id == video_id)
        m_last_requested = ImGui::GetTime();
    return get_frame_impl(video_id, out_w, out_h);
}

}
