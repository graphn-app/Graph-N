#include "ui/helpers/video_control_win32.h"
#ifdef _WIN32
#include "ui/helpers/textures.h"

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <wrl/client.h>

#include "stb_image_write.h"

using Microsoft::WRL::ComPtr;

namespace platform
{

VideoControl_Win32::VideoControl_Win32()
    : m_reader(nullptr, [](IMFSourceReader* p) { if (p) p->Release(); })
{
}

VideoControl_Win32::~VideoControl_Win32()
{
    if (m_initialized)
    {
        cleanup();
        MFShutdown();
    }
}

void VideoControl_Win32::init()
{
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        LogWarn("failed to initialize media foundation, err " << LogHRES(hr));
        return;
    }

    m_initialized = true;
}

void VideoControl_Win32::cleanup()
{
    if (!m_initialized)
        return;

    if (m_has_video)
    {
        m_stop_thread.store(true, std::memory_order_relaxed);
        m_cv.notify_all();

        if (m_decode_thread.joinable())
            m_decode_thread.join();

        m_reader.reset();
        m_info = {};

        m_has_video = false;
        m_is_playing.store(false, std::memory_order_relaxed);

        m_frame_buffer.reset();
        m_texture.reset();
    }
}

mngr::VideoInfo VideoControl_Win32::get_info(const std::string& filepath, std::optional<bool>& valid)
{
    if (!m_initialized)
        return {};

    valid = false;
    std::filesystem::path wfilepath(filepath);

    ComPtr<IMFSourceReader> reader;
    HRESULT hr = MFCreateSourceReaderFromURL(wfilepath.c_str(), nullptr, &reader);
    if (FAILED(hr))
        return {};

    mngr::VideoInfo info;

    PROPVARIANT var;
    PropVariantInit(&var);
    hr = reader->GetPresentationAttribute(cast::to_u32(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &var);
    if (SUCCEEDED(hr))
    {
        info.duration_sec = static_cast<float>(var.uhVal.QuadPart) / 10000000.0f;
        PropVariantClear(&var);
    }

    ComPtr<IMFMediaType> media_type;
    hr = reader->GetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &media_type);
    if (SUCCEEDED(hr))
    {
        UINT32 width = 0, height = 0;
        hr = MFGetAttributeSize(media_type.Get(), MF_MT_FRAME_SIZE, &width, &height);
        if (SUCCEEDED(hr))
        {
            info.width = width;
            info.height = height;
        }

        UINT32 num = 0, den = 1;
        hr = MFGetAttributeRatio(media_type.Get(), MF_MT_FRAME_RATE, &num, &den);
        if (SUCCEEDED(hr) && den != 0)
        {
            info.fps = static_cast<float>(num) / static_cast<float>(den);
        }

        if (info.width > 0 && info.height > 0)
            valid = true;
    }

    return info;
}

static void scale(UINT32& size_w, UINT32& size_h, UINT32 max_size)
{
    if (size_w > max_size || size_h > max_size)
    {
        const float ratio = cast::to_float(size_w) / cast::to_float(size_h);
        if (size_w > size_h)
        {
            size_w = max_size;
            size_h = cast::to_u32(max_size / ratio);
        }
        else
        {
            size_h = max_size;
            size_w = cast::to_u32(max_size * ratio);
        }

        size_w = (size_w / 2) * 2;
        size_h = (size_h / 2) * 2;
    }
}

int32_t VideoControl_Win32::load(const std::string& filepath, uint32_t max_side_len)
{
    if (!m_initialized)
        return 0;

    cleanup();

    HRESULT hr = S_OK;
    const auto log_and_reset_err = [this, &hr, &filepath](const char* err)
    {
        cleanup();
        Log(err << " '" << filepath << "', err " << LogHRES(hr));
        return 0;
    };

    ComPtr<IMFAttributes> attributes;
    hr = MFCreateAttributes(&attributes, /*3*/2);
    if (FAILED(hr)) return log_and_reset_err("Failed to create attributes for");

    hr = attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    if (FAILED(hr)) return log_and_reset_err("Failed to set attr hardware transforms for");

    hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
    if (FAILED(hr)) return log_and_reset_err("Failed to set attr video processing for");
/*
    ComPtr<IMFDXGIDeviceManager> dxgi_manager;
    UINT reset_token = 0;
    hr = MFCreateDXGIDeviceManager(&reset_token, &dxgi_manager);
    if (FAILED(hr)) return log_and_reset_err("Failed to create dxgi device manager for");

    hr = dxgi_manager->ResetDevice(m_device, reset_token);
    if (FAILED(hr)) return log_and_reset_err("Failed to reset device for");

    hr = attributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, dxgi_manager.Get());
    if (FAILED(hr)) return log_and_reset_err("Failed to set unknown device for");
*/
    std::filesystem::path wfilepath(filepath);
    IMFSourceReader* reader = nullptr;
    hr = MFCreateSourceReaderFromURL(wfilepath.c_str(), attributes.Get(), &reader);
    if (FAILED(hr)) return log_and_reset_err("Failed to create reader from");

    m_reader.reset(reader);

    ComPtr<IMFMediaType> native_type;
    hr = m_reader->GetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &native_type);
    if (FAILED(hr)) return log_and_reset_err("Failed to set first stream reader for");

    UINT32 w = 0, h = 0;
    hr = MFGetAttributeSize(native_type.Get(), MF_MT_FRAME_SIZE, &w, &h);
    if (FAILED(hr)) return log_and_reset_err("Failed to get attr size for");

    scale(w, h, max_side_len);

    ComPtr<IMFMediaType> target_type;
    hr = MFCreateMediaType(&target_type);
    if (FAILED(hr)) return log_and_reset_err("Failed to create media type for");

    hr = native_type->CopyAllItems(target_type.Get());
    if (FAILED(hr)) return log_and_reset_err("failed to copy items to target type for");

    //hr = target_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    //if (FAILED(hr)) return log_and_reset_err("Failed to set guid: major type for");

    hr = target_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (FAILED(hr)) return log_and_reset_err("failed to set rgb32 for");

    hr = MFSetAttributeSize(target_type.Get(), MF_MT_FRAME_SIZE, w, h);
    if (FAILED(hr)) return log_and_reset_err("failed to set new frame size for");

    target_type->DeleteItem(MF_MT_DEFAULT_STRIDE);
    target_type->DeleteItem(MF_MT_SAMPLE_SIZE);
    target_type->DeleteItem(MF_MT_FIXED_SIZE_SAMPLES);
    target_type->DeleteItem(MF_MT_MINIMUM_DISPLAY_APERTURE);
    target_type->DeleteItem(MF_MT_PAN_SCAN_APERTURE);
    target_type->DeleteItem(MF_MT_GEOMETRIC_APERTURE);

    //hr = target_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    //if (FAILED(hr)) return log_and_reset_err("Failed to set attr interlace mode for");

    //hr = MFSetAttributeRatio(target_type.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    //if (FAILED(hr)) return log_and_reset_err("Failed to set attr ratio for");

    hr = m_reader->SetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, target_type.Get());
    if (FAILED(hr)) return log_and_reset_err("Failed to set curr media type for");

    hr = m_reader->SetStreamSelection(cast::to_u32(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    if (FAILED(hr)) return log_and_reset_err("Failed to set stream selection: reader all for");

    hr = m_reader->SetStreamSelection(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
    if (FAILED(hr)) return log_and_reset_err("Failed to set stream selection: first stream for");

    std::optional<bool> valid;
    m_info = get_info(filepath, valid);
    if (!valid.value_or(false))
        return log_and_reset_err("Failed to get video info for ");

    m_info.width = w;
    m_info.height = h;
    m_frame_buffer.reset(new byte[h * w * 4], [](byte* p) { if (p) delete[] p; });

    m_video_id = ++m_last_video_id;
    m_has_video = true;
    m_is_playing.store(false, std::memory_order_relaxed);
    m_current_time.store(0.0f, std::memory_order_relaxed);
    m_seek_requested.store(false, std::memory_order_relaxed);
    m_frame_ready.store(false, std::memory_order_relaxed);
    m_stop_thread.store(false, std::memory_order_relaxed);
    m_last_frame_request.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

    Log("Loaded video id=" << m_video_id << " from " << filepath);

    seek(m_video_id, 0.0f);

    m_decode_thread = std::thread(&VideoControl_Win32::decode_thread_loop, this, m_info);

    return m_video_id;
}

void VideoControl_Win32::play(int32_t video_id)
{
    if (m_video_id != video_id)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Video must be loaded first");
    }

    if (!m_initialized || !m_has_video || m_is_playing.load(std::memory_order_relaxed))
        return;

    // rewind only if we reached the end of the file
    if (m_current_time.load(std::memory_order_relaxed) >= m_info.duration_sec)
        seek(video_id, 0.0f);

    m_is_playing.store(true, std::memory_order_relaxed);
    m_cv.notify_all();
}

void VideoControl_Win32::stop(int32_t video_id)
{
    if (m_video_id != video_id)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Video must be loaded first");
    }

    if (!m_initialized || !m_has_video)
        return;

    m_is_playing.store(false, std::memory_order_relaxed);
}

void VideoControl_Win32::seek(int32_t video_id, float pos)
{
    if (m_video_id != video_id)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Video must be loaded first");
    }

    if (!m_initialized || !m_has_video)
        return;

    if (pos > m_info.duration_sec) pos = m_info.duration_sec;
    else if (pos < 0.0f) pos = 0.0f;

    m_seek_target.store(pos, std::memory_order_relaxed);
    m_seek_requested.store(true, std::memory_order_relaxed);
    m_cv.notify_all();
}

bool VideoControl_Win32::get_is_playing(int32_t video_id) const
{
    if (!m_initialized || !m_has_video || m_video_id != video_id)
        return false;
    return m_is_playing.load(std::memory_order_relaxed);
}

float VideoControl_Win32::get_position(int32_t video_id) const
{
    if (!m_initialized || !m_has_video || m_video_id != video_id)
        return 0.0f;
    return m_current_time.load(std::memory_order_relaxed);
}

ImTextureID VideoControl_Win32::get_frame(int32_t video_id, float* out_w /*= nullptr*/, float* out_h /*= nullptr*/)
{
    if (m_video_id != video_id)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Video must be loaded first");
    }

    if (!m_initialized || !m_has_video)
        return nullptr;

    m_last_frame_request.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
    if (m_frame_ready.load(std::memory_order_relaxed))
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_texture) // first frame
        {
            m_texture = g::pTextures->upload_to_gpu(m_frame_buffer, m_info.width, m_info.height, true);
        }
        else // next frames
        {
            g::pTextures->update_gpu_texture(m_texture, m_frame_buffer, m_info.width, m_info.height, true);
        }

        m_frame_ready.store(false, std::memory_order_relaxed);
    }

    if (out_w) *out_w = cast::to_float(m_info.width);
    if (out_h) *out_h = cast::to_float(m_info.height);
    return m_texture.get(); // returns updated frame or previous frame or null
}

void VideoControl_Win32::decode_thread_loop(mngr::VideoInfo info)
{
    const int32_t frame_ms = info.fps > 0.0f ? cast::to_i32(1000.0f / info.fps) : 33;

    while (!m_stop_thread.load(std::memory_order_relaxed))
    {
        const bool seek_req = m_seek_requested.load(std::memory_order_relaxed);
        const bool is_playing = m_is_playing.load(std::memory_order_relaxed);
        const bool frame_needed = !m_frame_ready.load(std::memory_order_relaxed);
        const auto now = std::chrono::steady_clock::now();

        if (is_playing)
        {
            const auto last_req = m_last_frame_request.load(std::memory_order_relaxed);
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_req).count() > HIDDEN_VIDEO_PLAYING)
            {
                m_is_playing.store(false, std::memory_order_relaxed);
                continue;
            }
        }

        if (!is_playing && !seek_req && !frame_needed)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]()
                {
                    return
                        m_is_playing.load(std::memory_order_relaxed) || 
                        m_seek_requested.load(std::memory_order_relaxed) || 
                        m_stop_thread.load(std::memory_order_relaxed);
                });
            if (m_stop_thread.load(std::memory_order_relaxed))
                break;
            continue;
        }

        if (m_seek_requested.load(std::memory_order_relaxed))
        {
            const float target = m_seek_target.load(std::memory_order_relaxed);
            PROPVARIANT var;
            HRESULT hr = InitPropVariantFromInt64(cast::to_i64(target * 10000000.0f), &var);
            if (SUCCEEDED(hr))
            {
                m_reader->SetCurrentPosition(GUID_NULL, var);
                PropVariantClear(&var);
            }        
            m_seek_requested.store(false, std::memory_order_relaxed);
            m_frame_ready.store(false, std::memory_order_relaxed);    
        }

        const auto frame_start = std::chrono::steady_clock::now();

        if (m_is_playing.load(std::memory_order_relaxed) || !m_frame_ready.load(std::memory_order_relaxed))
        {
            ComPtr<IMFSample> sample;
            DWORD flags = 0;
            LONGLONG timestamp = 0;

            HRESULT hr = m_reader->ReadSample(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), 0, nullptr, &flags, &timestamp, &sample);
            if (FAILED(hr))
            {
                m_is_playing.store(false, std::memory_order_relaxed);
                continue;
            }

            if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                m_is_playing.store(false, std::memory_order_relaxed);
                m_current_time.store(info.duration_sec, std::memory_order_relaxed);
                m_frame_ready.store(true, std::memory_order_relaxed);
                continue;
            }

            if (sample)
            {
                ComPtr<IMFMediaBuffer> buffer;
                hr = sample->ConvertToContiguousBuffer(&buffer);
                if (SUCCEEDED(hr))
                {
                    BYTE* data = nullptr;
                    DWORD length = 0;

                    hr = buffer->Lock(&data, nullptr, &length);
                    if (SUCCEEDED(hr))
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);

                        const size_t pixels = info.width * info.height;
                        const uint32_t* src = cast::ptrs<uint32_t>(data);
                        uint32_t* dst = cast::ptrs<uint32_t>(m_frame_buffer.get());

                        for (size_t i = 0; i < pixels; ++i)
                        {
                            uint32_t pixel = src[i];
                            dst[i] = (pixel & 0xFF00FF00) | ((pixel & 0x00FF0000) >> 16) | ((pixel & 0x000000FF) << 16);
                        }

                        buffer->Unlock();

                        m_current_time.store(cast::to_float(timestamp) / 10000000.0f, std::memory_order_relaxed);
                        m_frame_ready.store(true, std::memory_order_relaxed);
                    }
                }
            }
        }

        if (m_is_playing.load(std::memory_order_relaxed))
        {
            const auto decode_duration = std::chrono::steady_clock::now() - frame_start;
            const auto time_to_sleep = std::chrono::milliseconds(frame_ms) - decode_duration;

            if (time_to_sleep > 0ms)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait_for(lock, time_to_sleep, [this]()
                    {
                        return
                            m_seek_requested.load(std::memory_order_relaxed) || 
                            !m_is_playing.load(std::memory_order_relaxed) ||
                            m_stop_thread.load(std::memory_order_relaxed);
                    });
            }
        }
    }
}

std::string VideoControl_Win32::get_thumbnail(const std::string& filename, uint32_t max_side_len)
{
    if (!m_initialized)
        return {};

    if (!tools::path::exists(filename))
        return {};

    const auto temp = fs::get_temp_directory_path();
    if (!tools::path::make_folder(temp))
        return {};

    HRESULT hr = S_OK;
    const auto log_err = [this, &hr, &filename](const char* err)
    {
        Log(err << " '" << filename << "', err " << LogHRES(hr));
        return std::string();
    };

    ComPtr<IMFAttributes> attributes;
    hr = MFCreateAttributes(&attributes, 2);
    if (FAILED(hr)) return log_err("Failed to create attributes for");

    hr = attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    if (FAILED(hr)) return log_err("Failed to set attr hardware transforms for");

    hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
    if (FAILED(hr)) return log_err("Failed to set attr video processing for");

    auto dest = tools::str::generate_uuid_v4() + ".png";
    dest = (temp / dest).u8string();
    std::filesystem::path wfilepath(filename);

    IMFSourceReader* reader = nullptr;
    hr = MFCreateSourceReaderFromURL(wfilepath.c_str(), attributes.Get(), &reader);
    if (FAILED(hr)) return log_err("Failed to create reader from");

    std::unique_ptr<IMFSourceReader, void(*)(IMFSourceReader*)> temp_reader(reader, [](IMFSourceReader* p) { if (p) p->Release(); });

    ComPtr<IMFMediaType> native_type;
    hr = temp_reader->GetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &native_type);
    if (FAILED(hr)) return log_err("Failed to set first stream reader for");

    UINT32 w = 0, h = 0;
    hr = MFGetAttributeSize(native_type.Get(), MF_MT_FRAME_SIZE, &w, &h);
    if (FAILED(hr)) return log_err("Failed to get attr size for");

    scale(w, h, max_side_len);

    // configure reader output to uncompressed rgb32 with target dimensions
    ComPtr<IMFMediaType> target_type;
    hr = MFCreateMediaType(&target_type);
    if (FAILED(hr)) return log_err("Failed to create media type for");

    hr = native_type->CopyAllItems(target_type.Get());
    if (FAILED(hr)) return log_err("failed to copy items to target type for");

    //hr = target_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    //if (FAILED(hr)) return log_err("Failed to set guid: major type for");

    hr = target_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (FAILED(hr)) return log_err("failed to set rgb32 for");

    hr = MFSetAttributeSize(target_type.Get(), MF_MT_FRAME_SIZE, w, h);
    if (FAILED(hr)) return log_err("failed to set new frame size for");

    target_type->DeleteItem(MF_MT_DEFAULT_STRIDE);
    target_type->DeleteItem(MF_MT_SAMPLE_SIZE);
    target_type->DeleteItem(MF_MT_FIXED_SIZE_SAMPLES);
    target_type->DeleteItem(MF_MT_MINIMUM_DISPLAY_APERTURE);
    target_type->DeleteItem(MF_MT_PAN_SCAN_APERTURE);
    target_type->DeleteItem(MF_MT_GEOMETRIC_APERTURE);

    //hr = target_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    //if (FAILED(hr)) return log_err("Failed to set attr interlace mode for");

    //hr = MFSetAttributeRatio(target_type.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    //if (FAILED(hr)) return log_err("Failed to set attr ratio for");

    hr = temp_reader->SetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, target_type.Get());
    if (FAILED(hr)) return log_err("Failed to set curr media type for");

    hr = temp_reader->SetStreamSelection(cast::to_u32(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    if (FAILED(hr)) return log_err("Failed to set stream selection: reader all for");

    hr = temp_reader->SetStreamSelection(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
    if (FAILED(hr)) return log_err("Failed to set stream selection: first stream for");

    // synchronous loop to extract the first non-empty visual frame
    ComPtr<IMFSample> sample;
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    bool frame_extracted = false;
    std::vector<byte> output_pixels(w * h * 4);

    while (!frame_extracted)
    {
        hr = temp_reader->ReadSample(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), 0, nullptr, &flags, &timestamp, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM))
            return log_err("Failed to read first stream sample for");

        if (sample)
        {
            ComPtr<IMFMediaBuffer> buffer;
            hr = sample->ConvertToContiguousBuffer(&buffer);
            if (SUCCEEDED(hr))
            {
                BYTE* data = nullptr;
                DWORD length = 0;
                hr = buffer->Lock(&data, nullptr, &length);
                if (SUCCEEDED(hr))
                {
                    const size_t pixels = w * h;
                    const uint32_t* src = cast::ptrs<uint32_t>(data);
                    uint32_t* dst = cast::ptrs<uint32_t>(output_pixels.data());

                    // swizzle bgra to rgba (im_col32 format)
                    for (size_t i = 0; i < pixels; ++i)
                    {
                        uint32_t pixel = src[i];
                        dst[i] = (pixel & 0xFF00FF00) | ((pixel & 0x00FF0000) >> 16) | ((pixel & 0x000000FF) << 16);
                    }

                    buffer->Unlock();
                    frame_extracted = true;
                }
            }
        }
    }

    // write the raw rgba buffer to disk using stb_image_write
    const auto write_res = stbi_write_png(dest.c_str(), cast::to_i32(w), cast::to_i32(h), 4, output_pixels.data(), cast::to_i32(w * 4));
    if (0 == write_res || !tools::path::exists(dest))
    {
        hr = write_res;
        return log_err("Failed to write stb png for");
    }

    return dest;
}

} // namespace platform

#endif // _WIN32
