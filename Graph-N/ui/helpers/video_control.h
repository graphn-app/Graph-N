#pragma once
#include "core/base/structs.h"

namespace global
{

class VideoControl
{
public:
    VideoControl() = default;
    virtual ~VideoControl();

    NO_COPY_MOVE(VideoControl);

    virtual void init() = 0;
    virtual void cleanup() = 0;

    virtual mngr::VideoInfo get_info(const std::string& filepath, std::optional<bool>& valid) = 0;

    inline int32_t get_current() const { return m_video_id; }

    virtual int32_t load(const std::string& filepath, uint32_t max_side_len) = 0;
    virtual void play(int32_t video_id) = 0;
    virtual void stop(int32_t video_id) = 0;
    virtual void seek(int32_t video_id, float pos) = 0;

    virtual bool get_is_playing(int32_t video_id) const = 0;
    virtual float get_position(int32_t video_id) const = 0;

    virtual ImTextureID get_frame(int32_t video_id, float* out_w = nullptr, float* out_h = nullptr) = 0;
    virtual std::string get_thumbnail(const std::string& filename, uint32_t max_side_len) = 0; // loads first valid frame and saves to temp. returns saved file path

protected:
    bool m_initialized = false;

    int32_t m_last_video_id = 0;
    int32_t m_video_id = 0;
};

} // namespace global
