
#pragma once

namespace mngr
{
struct ComfyUiObjectValidityListener
{
    ComfyUiObjectValidityListener() = default;
    virtual ~ComfyUiObjectValidityListener() = default;

    NO_COPY_MOVE(ComfyUiObjectValidityListener);

    virtual void invalidate_comfyui_objects() = 0;
    virtual void resolve_comfyui_objects() = 0;
};

}
