#pragma once
#ifdef __linux__
#include "ui/helpers/video_control.h"

namespace platform
{

class VideoControl_Linux final : public global::VideoControl
{
public:
    VideoControl_Linux() = default;
    ~VideoControl_Linux() override;
};

} // namespace platform

#endif // __linux__
