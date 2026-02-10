
#pragma once
#ifdef _WIN32
#include "ui/textures.h"

struct ID3D11Device;

namespace platform
{

class Textures_Win32 final : public Textures
{
public:
    Textures_Win32() = default;
    ~Textures_Win32() override = default;

    void set_main_routine(MainRoutine* main) override;
    ImTextureIDPtr upload_to_gpu(const ImageData& image_data, uint32_t width, uint32_t height, bool generate_mips) override;

private:
    ID3D11Device* m_device = nullptr;
};

} // namespace platform

#endif // #ifdef _WIN32
