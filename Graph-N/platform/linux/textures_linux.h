
#pragma once
#ifdef __linux__

#include "platform/textures.h"

namespace platform
{

class Textures_Linux final : public Textures
{
public:
    Textures_Linux() = default;
    ~Textures_Linux() override = default;
};

} // namespace platform

#endif // #ifdef __linux__
