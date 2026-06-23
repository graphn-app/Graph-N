
#pragma once
#ifdef __APPLE__

#include "ui/helpers/textures.h"

namespace platform
{

class Textures_Apple final : public global::Textures
{
public:
    Textures_Apple() = default;
    ~Textures_Apple() override = default;
};

} // namespace platform

#endif // #ifdef __APPLE__
