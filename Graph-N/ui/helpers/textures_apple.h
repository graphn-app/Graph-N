/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

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
