
#pragma once
#ifdef _WIN32
#include "platform/textures.h"

struct ID3D11Device;

namespace platform
{

class Textures_Win32 final : public Textures
{
public:
    Textures_Win32() = default;
    ~Textures_Win32() override = default;

private:
    ID3D11Device* get_device() const;
};

} // namespace platform

#endif // #ifdef _WIN32
