/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#ifdef __linux__
#include "ui/helpers/textures.h"

namespace platform
{

class Textures_Linux final : public global::Textures
{
public:
    Textures_Linux() noexcept = default;
    ~Textures_Linux() noexcept override = default;
};

} // namespace platform

#endif // #ifdef __linux__
