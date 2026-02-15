
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

struct Lazy1sTimer
{
    Lazy1sTimer() = default;
    virtual ~Lazy1sTimer() = 0;

    NO_COPY_MOVE(Lazy1sTimer);

    virtual void on_1s_timer(int64_t seconds_elapsed) = 0;
};

inline Lazy1sTimer::~Lazy1sTimer() = default;

} // namespace listener
