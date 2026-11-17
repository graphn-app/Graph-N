/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef __linux__
#include "ui/helpers/video_control_linux.h"

namespace platform
{

VideoControl_Linux::~VideoControl_Linux() noexcept = default;

void VideoControl_Linux::init()
{
    m_initialized = true;
}

void VideoControl_Linux::stop_and_cleanup()
{
    cleanup_current();
    m_initialized = false;
}

mngr::VideoInfo VideoControl_Linux::get_info(const std::filesystem::path& /*filepath*/, uint32_t /*max_side_len*/, std::optional<bool>& valid)
{
    valid = false;
    return mngr::VideoInfo();
}

int32_t VideoControl_Linux::load(const std::filesystem::path& /*filepath*/, uint32_t /*max_side_len*/)
{
    return -1;
}

void VideoControl_Linux::seek(int32_t /*video_id*/, float /*pos*/)
{
}

bool VideoControl_Linux::get_is_playing(int32_t /*video_id*/) const noexcept
{
    return false;
}

float VideoControl_Linux::get_position(int32_t /*video_id*/) const noexcept
{
    return 0.0f;
}

void VideoControl_Linux::cleanup_current()
{
}

void VideoControl_Linux::play_impl(int32_t /*video_id*/)
{
}

void VideoControl_Linux::stop_impl(int32_t /*video_id*/)
{
}

ImTextureID VideoControl_Linux::get_frame_impl(int32_t /*video_id*/, float* /*out_w*/, float* /*out_h*/)
{
    return nullptr;
}

} // namespace platform

#endif // __linux__
