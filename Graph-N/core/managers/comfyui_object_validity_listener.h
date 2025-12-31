
#pragma once

namespace mngr
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

}
