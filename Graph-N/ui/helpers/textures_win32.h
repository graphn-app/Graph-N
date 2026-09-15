#pragma once
#ifdef _WIN32
#include "ui/helpers/textures.h"

struct ID3D11Device;

namespace platform
{

class Textures_Win32 final : public global::Textures
{
public:
    Textures_Win32() noexcept = default;
    ~Textures_Win32() noexcept override = default;

    ImTextureIDPtr upload_to_gpu(const byte* image_data, uint32_t width, uint32_t height, bool generate_mips) override;
    bool update_gpu_texture(ImTextureIDPtr texture, const byte* image_data, uint32_t width, uint32_t height, bool generate_mips) override;

private:
    ID3D11Device* m_device = nullptr;
};

} // namespace platform

#endif // #ifdef _WIN32
