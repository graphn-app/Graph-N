
#pragma once

namespace listener
{

struct ComfyUiObjectValidityListener
{
    ComfyUiObjectValidityListener() = default;
    virtual ~ComfyUiObjectValidityListener() = 0;

    NO_COPY_MOVE(ComfyUiObjectValidityListener);

    virtual void invalidate_comfyui_objects() {}
    virtual void begin_resolve_comfyui_objects() {}
    virtual void finalize_resolve_comfyui_objects() {}
};

inline ComfyUiObjectValidityListener::~ComfyUiObjectValidityListener() = default;

///////////////////////////

struct Timer
{
    Timer() = default;
    virtual ~Timer() = 0;

    NO_COPY_MOVE(Timer);

    virtual void on_1s_lazy_timer(int64_t total_seconds_elapsed) = 0;
    virtual void on_frame(int64_t total_frames_elapsed) = 0;
};

inline Timer::~Timer() = default;

} // namespace listener
