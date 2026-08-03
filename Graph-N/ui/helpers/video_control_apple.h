#pragma once
#ifdef __APPLE__
#include "ui/helpers/video_control.h"

namespace platform
{

class VideoControl_Apple final : public global::VideoControl
{
public:
    VideoControl_Apple() noexcept = default;
    ~VideoControl_Apple() noexcept override;
};

} // namespace platform

#endif // __APPLE__
