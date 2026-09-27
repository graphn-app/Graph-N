#pragma once
#include "core/alive.h"

namespace listener
{

struct ComfyUiObjectValidityListener
{
    ComfyUiObjectValidityListener() noexcept = default;
    virtual ~ComfyUiObjectValidityListener() noexcept = 0;

    NO_COPY_MOVE(ComfyUiObjectValidityListener);

    enum class Step : uint8_t { Invalidate, ResolveNodes, ResolvePins, ResolveWfs, ResolvePanels };

    virtual void invalidate_comfyui_objects() {}
    virtual void resolve_comfyui_objects(Step /*step*/) {}
};

inline ComfyUiObjectValidityListener::~ComfyUiObjectValidityListener() noexcept = default;

///////////////////////////

struct Timer
{
    Timer() noexcept = default;
    virtual ~Timer() noexcept = 0;

    NO_COPY_MOVE(Timer);

    virtual void on_1s_lazy_timer(int64_t total_seconds_elapsed) {}
    virtual void on_frame(int64_t total_frames_elapsed) {}
};

inline Timer::~Timer() noexcept = default;

///////////////////////////

struct ServerRestartHandler
{
    ServerRestartHandler() noexcept = default;
    virtual ~ServerRestartHandler() noexcept = default;
    virtual void handle_server_killed() {}
};

///////////////////////////

struct HotkeysChangedHandler
{
    HotkeysChangedHandler() noexcept = default;
    virtual ~HotkeysChangedHandler() noexcept = default;
    virtual void handle_hotkeys_changed() = 0;
};

} // namespace listener
