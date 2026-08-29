#pragma once
#ifdef __APPLE__
#include "ui/helpers/textures.h"

namespace platform
{

class Textures_Apple final : public global::Textures
{
public:
    Textures_Apple() noexcept = default;
    ~Textures_Apple() noexcept override = default;
};

} // namespace platform

#endif // #ifdef __APPLE__
