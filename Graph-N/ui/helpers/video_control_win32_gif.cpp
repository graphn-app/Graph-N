#include "ui/helpers/video_control_win32_gif.h"
#ifdef _WIN32
#include "ui/helpers/textures.h"

#pragma comment(lib, "windowscodecs.lib")

namespace platform
{

VideoControl_Win32_Gif::VideoControl_Win32_Gif() noexcept
{
}

VideoControl_Win32_Gif::~VideoControl_Win32_Gif() noexcept
{
    try {
        m_factory.Reset();
        cleanup();
    }
    catch(...) {}
}

// Helper to extract WIC metadata safely
template<typename T>
T get_wic_meta(::IWICMetadataQueryReader* meta, const wchar_t* path, T def) 
{
    if (!meta)
        return def;
    ::PROPVARIANT prop;
    ::PropVariantInit(&prop);
    T res = def;
    if (SUCCEEDED(meta->GetMetadataByName(path, &prop))) 
    {
        if (prop.vt == VT_UI2) res = cast::to<T>(prop.uiVal);
        else if (prop.vt == VT_UI1) res = cast::to<T>(prop.bVal);
        ::PropVariantClear(&prop);
    }
    return res;
}

bool VideoControl_Win32_Gif::path_is_gif(const std::filesystem::path& filepath) const noexcept
{
    const auto low = tools::str::to_lower_ascii(tools::path::path_to_string(filepath.extension()));
    return low == ".gif";
}

bool VideoControl_Win32_Gif::init() noexcept
{
    const auto hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr) || !m_factory)
    {
        LogWarn("Failed to initialize WICImagingFactory, err: " << LogHRES(hr));
        return false;
    }
    m_initialized = true;
    return true;
}

void VideoControl_Win32_Gif::cleanup()
{
    m_is_playing = false;
    m_current_time = 0.0f;
    m_current_frame_idx = -1;

    m_decoder.Reset();
    m_canvas.clear();
    m_backup_canvas.clear();
    m_frames_meta.clear();

    m_info = {};
    m_info.is_gif = true;
    m_texture = nullptr; 
}

mngr::VideoInfo VideoControl_Win32_Gif::get_info(const std::filesystem::path& filepath, std::optional<bool>& valid) 
{
    valid = false;
    if (!m_initialized)
        return {};

    ComPtr<::IWICBitmapDecoder> decoder;
    HRESULT hr = m_factory->CreateDecoderFromFilename(filepath.wstring().c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr))  return {};

    uint32_t frame_count = 0;
    hr = decoder->GetFrameCount(&frame_count);
    if (FAILED(hr) || 0 == frame_count) return {};

    ComPtr<::IWICBitmapFrameDecode> first_frame;
    hr = decoder->GetFrame(0, &first_frame);
    if (FAILED(hr)) return {};

    UINT w, h;
    hr = first_frame->GetSize(&w, &h);
    if (FAILED(hr)) return {};

    mngr::VideoInfo info;
    info.is_gif = true;
    info.width = cast::to_i32(w);
    info.height = cast::to_i32(h);

    float duration = 0.0f;
    for (uint32_t i = 0; i < frame_count; ++i) 
    {
        ComPtr<::IWICBitmapFrameDecode> f;
        if (SUCCEEDED(decoder->GetFrame(i, &f))) 
        {
            ComPtr<::IWICMetadataQueryReader> meta;
            f->GetMetadataQueryReader(&meta);

            uint16_t delay = get_wic_meta<uint16_t>(meta.Get(), L"/grctlext/Delay", 10);
            if (delay <= 1) delay = 10; 
            duration += cast::to_float(delay * 10) / 1000.0f;
        }
    }

    info.duration_sec = duration;
    info.fps = duration > 0.0f ? cast::to_float(frame_count) / duration : 0.0f;    

    // Generate thumbnail
    ComPtr<::IWICFormatConverter> converter;
    m_factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(converter->Initialize(first_frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
    {
        std::vector<uint32_t> pixels(info.width * info.height);
        converter->CopyPixels(nullptr, info.width * 4, info.width * info.height * 4, cast::ptrs<BYTE>(pixels.data()));

        auto temp_thumb = tools::path::path_to_string(fs::get_temp_filepath(".png"));
        if (stbi_write_png(temp_thumb.c_str(), info.width, info.height, 4, pixels.data(), info.width * 4) != 0)
        {
            valid = true;
            info.thumbnail = std::move(temp_thumb);
        }
    }

    return info;
}

bool VideoControl_Win32_Gif::load(const std::filesystem::path& filepath) 
{
    cleanup();
    if (!m_initialized)
        return false;

    HRESULT hr = m_factory->CreateDecoderFromFilename(filepath.wstring().c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &m_decoder);
    if (FAILED(hr)) return false;

    uint32_t frame_count = 0;
    hr = m_decoder->GetFrameCount(&frame_count);
    if (FAILED(hr) || 0 == frame_count) return false;

    ComPtr<::IWICBitmapFrameDecode> first_frame;
    hr = m_decoder->GetFrame(0, &first_frame);
    if (FAILED(hr)) return false;

    UINT w, h;
    hr = first_frame->GetSize(&w, &h);
    if (FAILED(hr)) return false;

    m_info.width = cast::to_i32(w);
    m_info.height = cast::to_i32(h);

    m_frames_meta.reserve(frame_count);
    float accum_time = 0.0f;

    for (uint32_t i = 0; i < frame_count; ++i) 
    {
        ComPtr<::IWICBitmapFrameDecode> frame;
        hr = m_decoder->GetFrame(i, &frame);
        if (FAILED(hr)) return false;

        ComPtr<::IWICMetadataQueryReader> meta;
        hr = frame->GetMetadataQueryReader(&meta);
        if (FAILED(hr)) return false;

        FrameMeta fm = {};
        hr = frame->GetSize(&fm.width, &fm.height);
        if (FAILED(hr)) return false;

        fm.left = get_wic_meta<uint16_t>(meta.Get(), L"/imgdesc/Left", 0);
        fm.top  = get_wic_meta<uint16_t>(meta.Get(), L"/imgdesc/Top", 0);
        fm.disposal = get_wic_meta<uint8_t>(meta.Get(), L"/grctlext/Disposal", 0);
        fm.delay_ms = get_wic_meta<uint16_t>(meta.Get(), L"/grctlext/Delay", 10) * 10;
        if (fm.delay_ms <= 10)
            fm.delay_ms = 100;

        fm.timestamp_sec = accum_time;
        accum_time += cast::to_float(fm.delay_ms) / 1000.0f;

        m_frames_meta.push_back(fm);
    }

    m_info.duration_sec = accum_time;
    m_info.fps = frame_count > 0 ? cast::to_float(frame_count) / accum_time : 0.0f;

    m_canvas.assign(m_info.width * m_info.height, 0);

    return true;
}

void VideoControl_Win32_Gif::play() 
{
    if (m_decoder)
    {
        if (m_current_time >= m_info.duration_sec) 
            seek(0.0f);
        m_is_playing = true;
    }
}

void VideoControl_Win32_Gif::stop() 
{
    cleanup();
}

void VideoControl_Win32_Gif::seek(float pos) 
{
    if (m_decoder)
    {
        // Allow only rewind to zero to avoid CPU lockup.
        if (pos <= 0.0f) 
        {
            m_current_time = 0.0f;
            m_current_frame_idx = -1;
            std::fill(m_canvas.begin(), m_canvas.end(), 0);
            m_backup_canvas.clear();
        }
    }
}

void VideoControl_Win32_Gif::apply_frame(int32_t idx) 
{
    if (idx < 0 || cast::to_szt(idx) >= m_frames_meta.size())
        return;

    const auto& meta = m_frames_meta[idx];

    // process disposal of the previous frame before drawing current
    if (idx > 0)
    {
        const auto& prev_meta = m_frames_meta[idx - 1];
        if (prev_meta.disposal == 2) // Restore to background (transparent)
        {
            for (uint32_t y = 0; y < prev_meta.height; ++y)
            {
                if (prev_meta.top + y >= cast::to_u32(m_info.height))
                    break;
                for (uint32_t x = 0; x < prev_meta.width; ++x)
                {
                    if (prev_meta.left + x >= cast::to_u32(m_info.width))
                        break;
                    m_canvas[(prev_meta.top + y) * m_info.width + (prev_meta.left + x)] = 0; 
                }
            }
        }
        else if (prev_meta.disposal == 3) // Restore to previous valid state
        {
            m_canvas = m_backup_canvas; 
        }
    }

    // Save backup if CURRENT frame requires it for the NEXT frame
    if (meta.disposal == 3) 
        m_backup_canvas = m_canvas;

    // Decode current frame delta
    ComPtr<::IWICBitmapFrameDecode> frame;
    if (FAILED(m_decoder->GetFrame(idx, &frame)))
        return;

    // Force conversion to 32bpp RGBA (perfect match for IM_COL32)
    ComPtr<::IWICFormatConverter> converter;
    m_factory->CreateFormatConverter(&converter);
    converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

    std::vector<uint32_t> pixels(meta.width * meta.height);
    converter->CopyPixels(nullptr, meta.width * 4, meta.width * meta.height * 4, cast::ptrs<BYTE>(pixels.data()));

    // Alpha blend onto main canvas
    for (uint32_t y = 0; y < meta.height; ++y) 
    {
        if (meta.top + y >= cast::to_u32(m_info.height))
            break;

        uint32_t* dst_row = m_canvas.data() + ((meta.top + y) * m_info.width + meta.left);
        const uint32_t* src_row = pixels.data() + (y * meta.width);

        for (uint32_t x = 0; x < meta.width; ++x) 
        {
            if (meta.left + x >= cast::to_u32(m_info.width))
                break;

            const uint32_t src = src_row[x];
            const uint32_t sa = (src >> 24) & 0xFF; 

            if (sa == 255) // Full opaque
                dst_row[x] = src; 
            else if (sa != 0) // Blend, 0 - Full transparent 
            {
                const uint32_t dst = dst_row[x];
                const uint32_t dr = dst & 0xFF;
                const uint32_t dg = (dst >> 8) & 0xFF;
                const uint32_t db = (dst >> 16) & 0xFF;
                const uint32_t da = dst >> 24;

                const uint32_t sr = src & 0xFF;
                const uint32_t sg = (src >> 8) & 0xFF;
                const uint32_t sb = (src >> 16) & 0xFF;

                const uint32_t out_r = (sr * sa + dr * (255 - sa)) / 255;
                const uint32_t out_g = (sg * sa + dg * (255 - sa)) / 255;
                const uint32_t out_b = (sb * sa + db * (255 - sa)) / 255;
                const uint32_t out_a = sa + da * (255 - sa) / 255;

                dst_row[x] = out_r | (out_g << 8) | (out_b << 16) | (out_a << 24);
            }
        }
    }
}

ImTextureID VideoControl_Win32_Gif::get_frame(float* out_w /*= nullptr*/, float* out_h /*= nullptr*/) 
{
    if (!m_initialized || !m_is_playing)
        return nullptr;

    if (out_w) *out_w = cast::to_float(m_info.width);
    if (out_h) *out_h = cast::to_float(m_info.height);

    if (m_is_playing)
    {
        m_current_time += ImGui::GetIO().DeltaTime;
        if (m_current_time >= m_info.duration_sec) 
        {
            seek(0.0f);
        }
    }

    int32_t target_frame = 0;
    for (int32_t i = cast::to_i32(m_frames_meta.size()) - 1; i >= 0; --i)
    {
        if (m_current_time >= m_frames_meta[i].timestamp_sec)
        {
            target_frame = i;
            break;
        }
    }

    if (target_frame > m_current_frame_idx)
    {
        // Sequentially apply all intermediate frames to composite correctly
        for (int32_t i = m_current_frame_idx + 1; i <= target_frame; ++i) {
            apply_frame(i);
        }
        m_current_frame_idx = target_frame;

        if (!m_texture) m_texture = g::pTextures->upload_to_gpu(cast::ptrs<byte>(m_canvas.data()), cast::to_u32(m_info.width), cast::to_u32(m_info.height), true);
        else g::pTextures->update_gpu_texture(m_texture, cast::ptrs<uint8_t>(m_canvas.data()), cast::to_u32(m_info.width), cast::to_u32(m_info.height), true);
    }

    return m_texture.get();
}

} // namespace platform

#endif // _WIN32
