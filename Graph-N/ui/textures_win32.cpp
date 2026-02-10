
#ifdef _WIN32
#include "ui/textures_win32.h"
#include "main/main_win32.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

namespace platform
{


void Textures_Win32::set_main_routine(MainRoutine* main)
{
    auto pmain = dynamic_cast<MainRoutine_Win32*>(main);
    m_device = pmain->get_device();
}

ImTextureIDPtr Textures_Win32::upload_to_gpu(const ImageData& image_data, uint32_t width, uint32_t height, bool generate_mips)
{
    if (!m_device || !image_data)
        return nullptr;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = cast::to_u32(width);
    desc.Height = cast::to_u32(height);
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    if (generate_mips)
    {
        desc.MipLevels = 0;
        desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    else
    {
        desc.MipLevels = 1;
        desc.MiscFlags = 0;
    }

    ID3D11Texture2D* texture_ptr = nullptr;
    HRESULT hr;

    if (!generate_mips)
    {
        D3D11_SUBRESOURCE_DATA init_data;
        ZeroMemory(&init_data, sizeof(init_data));
        init_data.pSysMem = image_data.get();
        init_data.SysMemPitch = desc.Width * 4;
        init_data.SysMemSlicePitch = 0;
        hr = m_device->CreateTexture2D(&desc, &init_data, &texture_ptr);
    }
    else
    {
        hr = m_device->CreateTexture2D(&desc, nullptr, &texture_ptr);
        if (SUCCEEDED(hr))
        {
            ID3D11DeviceContext* context = nullptr;
            m_device->GetImmediateContext(&context);
            context->UpdateSubresource(texture_ptr, 0, nullptr, image_data.get(), desc.Width * 4, 0);
            context->Release();
        }
    }

    if (FAILED(hr))
    {
        LogWarn("CreateTexture2D failed: " << LogHEX(hr));
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ZeroMemory(&srv_desc, sizeof(srv_desc));
    srv_desc.Format = desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = generate_mips ? -1 : 1; 

    ID3D11ShaderResourceView* srv_ptr = nullptr;
    hr = m_device->CreateShaderResourceView(texture_ptr, &srv_desc, &srv_ptr);
    if (SUCCEEDED(hr) && generate_mips)
    {
        ID3D11DeviceContext* context = nullptr;
        m_device->GetImmediateContext(&context);
        context->GenerateMips(srv_ptr);
        context->Release();
    }

    ImTextureIDPtr textures = std::shared_ptr<void>(srv_ptr, [](void* p) { ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)p; if (srv) srv->Release(); });
    texture_ptr->Release();
    return textures;
}

} // namespace platform

#endif // #ifdef _WIN32
