/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/helpers/video_control_win32.h"
#ifdef _WIN32
#include "ui/helpers/textures.h"
#include "ui/helpers/video_control_win32_gif.h"

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include <shobjidl.h>

#include "stb_image_write.h"

using Microsoft::WRL::ComPtr;

namespace platform
{

VideoControl_Win32::VideoControl_Win32() noexcept
    : m_reader(nullptr, [](::IMFSourceReader* p) { if (p) p->Release(); })
{
}

VideoControl_Win32::~VideoControl_Win32() noexcept
{
}

void VideoControl_Win32::stop_and_cleanup()
{
    if (m_initialized)
    {
        try
        {
            if (m_gif_player)
                m_gif_player->stop_and_cleanup();
            m_gif_player.reset();
            cleanup_current();
            ::MFShutdown();
        }
        catch (...) {}
    }

    VideoControl::stop_and_cleanup();
}

void VideoControl_Win32::init()
{
    const HRESULT hr = ::MFStartup(MF_VERSION);
    LogRAssert(SUCCEEDED(hr), "failed to initialize media foundation, err " << LogHRES(hr), );

    m_gif_player = std::make_unique<VideoControl_Win32_Gif>();
    m_initialized = m_gif_player->init();

    VideoControl::init();
}

void VideoControl_Win32::cleanup_current()
{
    if (!m_initialized)
        return;

    if (m_has_video)
    {
        m_stop_thread.request_stop();
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

namespace
{
void scale(UINT32& size_w, UINT32& size_h, UINT32 max_size) noexcept
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
} // namespace

mngr::VideoInfo VideoControl_Win32::get_info(const std::filesystem::path& filepath, uint32_t max_side_len, std::optional<bool>& valid)
{
    if (m_gif_player->path_is_gif(filepath))
        return m_gif_player->get_info(filepath, valid);

    valid = false;

    if (!m_initialized) [[unlikely]]
        return {};

    mngr::VideoInfo info;
    const auto dest_filename = fs::get_temp_filepath(".png");
    if (!dest_filename.empty())
    {
        auto dest_filename_str = tools::path::path_to_string(dest_filename);
        if (get_thumbnail_check_window_cache(filepath, dest_filename_str, max_side_len))
        {
            info.thumbnail = dest_filename_str;
        }
    }

    HRESULT hr = S_OK;
    const auto log_err = [&hr, &filepath, &info](const char* err) -> mngr::VideoInfo
    {
        LogWarnS(err << " '" << filepath << "', err " << LogHRES(hr));
        return info;
    };

    // setup Reader with Video Processing (needed for frame extraction)
    ComPtr<::IMFAttributes> attributes;
    hr = ::MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) return log_err("Failed to create attributes for");

    hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    if (FAILED(hr)) return log_err("Failed to set attr video processing for");

    ::IMFSourceReader* temp_reader_ptr = nullptr;
    hr = ::MFCreateSourceReaderFromURL(filepath.c_str(), attributes.Get(), &temp_reader_ptr);
    if (FAILED(hr)) return log_err("Failed to create reader from");
    std::unique_ptr<::IMFSourceReader, void(*)(::IMFSourceReader*)> reader(temp_reader_ptr, [](::IMFSourceReader* p) { if (p) p->Release(); });

    // extract Duration
    ::PROPVARIANT var;
    ::PropVariantInit(&var);
    hr = reader->GetPresentationAttribute(cast::to_u32(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &var);
    if (SUCCEEDED(hr))
    {
        info.duration_sec = cast::to_float(var.uhVal.QuadPart) / 10000000.0f;
        ::PropVariantClear(&var);
    }

    // extract original Width, Height, FPS from Native Media Type
    ComPtr<::IMFMediaType> native_type;
    hr = reader->GetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &native_type);
    if (FAILED(hr)) return log_err("Failed to get native media type for");

    UINT32 orig_w = 0, orig_h = 0;
    hr = ::MFGetAttributeSize(native_type.Get(), MF_MT_FRAME_SIZE, &orig_w, &orig_h);
    if (SUCCEEDED(hr))
    {
        info.width = orig_w;
        info.height = orig_h;
    }

    UINT32 num = 0, den = 1;
    hr = ::MFGetAttributeRatio(native_type.Get(), MF_MT_FRAME_RATE, &num, &den);
    if (SUCCEEDED(hr) && den != 0)
    {
        info.fps = cast::to_float(num) / cast::to_float(den);
    }

    if (info.width > 0 && info.height > 0)
        valid = true;

    // if we already got the thumbnail from cache, we are done
    if (!info.thumbnail.empty())
        return info;

    // 6. Otherwise, configure type and extract the first valid frame
    if (dest_filename.empty())
        return info; 

    const auto dest_filename_str = tools::path::path_to_string(dest_filename);

    UINT32 scaled_w = orig_w;
    UINT32 scaled_h = orig_h;
    scale(scaled_w, scaled_h, max_side_len);

    ComPtr<::IMFMediaType> target_type;
    hr = ::MFCreateMediaType(&target_type);
    if (FAILED(hr)) return log_err("Failed to create media type for");

    hr = target_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) return log_err("Failed to set major type for");

    hr = target_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (FAILED(hr)) return log_err("failed to set rgb32 for");

    hr = ::MFSetAttributeSize(target_type.Get(), MF_MT_FRAME_SIZE, scaled_w, scaled_h);
    if (FAILED(hr)) return log_err("failed to set new frame size for");

    hr = reader->SetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, target_type.Get());
    if (FAILED(hr)) return log_err("Failed to set curr media type for");

    hr = reader->SetStreamSelection(cast::to_u32(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    if (FAILED(hr)) return log_err("Failed to disable streams for");

    hr = reader->SetStreamSelection(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
    if (FAILED(hr)) return log_err("Failed to select first video stream for");

    ComPtr<::IMFSample> sample;
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    bool frame_extracted = false;
    std::vector<byte> output_pixels(scaled_w * scaled_h * 4);

    int32_t frames_read = 0;
    constexpr int32_t max_frames_to_read = 180;
    constexpr int32_t check_interval = 3;
    constexpr uint8_t luminance_threshold = 25;

    while (!frame_extracted && frames_read < max_frames_to_read)
    {
        hr = reader->ReadSample(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), 0, nullptr, &flags, &timestamp, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) 
            break;

        frames_read++;

        if (sample && (frames_read == 1 || (frames_read % check_interval) == 0))
        {
            ComPtr<::IMFMediaBuffer> buffer;
            hr = sample->ConvertToContiguousBuffer(&buffer);
            if (SUCCEEDED(hr))
            {
                BYTE* data = nullptr;
                DWORD length = 0;
                hr = buffer->Lock(&data, nullptr, &length);
                if (SUCCEEDED(hr))
                {
                    const size_t pixels = scaled_w * scaled_h;
                    const uint32_t* src = cast::ptrs<uint32_t>(data);
                    uint32_t* dst = cast::ptrs<uint32_t>(output_pixels.data());
                    uint32_t dark_pixels = 0;

                    for (size_t i = 0; i < pixels; ++i)
                    {
                        const uint32_t pixel = src[i];
                        dst[i] = 0xFF000000 | (pixel & 0x0000FF00) | ((pixel & 0x00FF0000) >> 16) | ((pixel & 0x000000FF) << 16);

                        const uint8_t b = (pixel & 0xFF);
                        const uint8_t g = ((pixel >> 8) & 0xFF);
                        const uint8_t r = ((pixel >> 16) & 0xFF);

                        if (r <= luminance_threshold && g <= luminance_threshold && b <= luminance_threshold)
                            dark_pixels++;
                    }

                    buffer->Unlock();

                    if (dark_pixels < (pixels * 7 / 10))
                        frame_extracted = true;
                }
            }
        }
    }

    if (!frame_extracted && 0 == frames_read)
        return log_err("Failed to extract any frames for");

    const auto write_res = stbi_write_png(dest_filename_str.c_str(), cast::to_i32(scaled_w), cast::to_i32(scaled_h), 4, output_pixels.data(), cast::to_i32(scaled_w * 4));
    if (0 == write_res || !tools::path::exists(dest_filename))
    {
        hr = write_res;
        return log_err("Failed to write stb png for");
    }

    info.thumbnail = dest_filename_str;
    return info;
}

int32_t VideoControl_Win32::load(const std::filesystem::path& filepath, uint32_t max_side_len)
{
    m_gif_playing = false;
    if (m_gif_player->path_is_gif(filepath))
    {
        if (!m_gif_player->load(filepath))
            return 0;

        m_gif_playing = true;
        m_video_id = ++m_last_video_id;
        return m_video_id;
    }

    if (!m_initialized)
        return 0;

    m_video_id = 0;
    cleanup_current();

    HRESULT hr = S_OK;
    const auto log_and_reset_err = [this, &filepath](const char* err, HRESULT hres)
    {
        cleanup_current();
        LogWarn(err << " '" << filepath << "', err " << LogHRES(hres));
        return 0;
    };

    UINT32 final_w = 0, final_h = 0;

    // helper to setup reader
    const auto try_setup_reader = [&](bool use_gpu) -> HRESULT
    {
        ComPtr<::IMFAttributes> attributes;
        HRESULT hr = ::MFCreateAttributes(&attributes, 3);
        if (FAILED(hr)) return hr;

        // always enable video processor for YUV -> RGB conversion
        attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

        if (use_gpu)
        {
            attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
            attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
        }

        ::IMFSourceReader* temp_reader = nullptr;
        hr = ::MFCreateSourceReaderFromURL(filepath.c_str(), attributes.Get(), &temp_reader);
        if (FAILED(hr)) return hr;

        std::unique_ptr<::IMFSourceReader, void(*)(::IMFSourceReader*)> safe_reader(temp_reader, [](::IMFSourceReader* p) { if (p) p->Release(); });

        ComPtr<::IMFMediaType> native_type;
        hr = safe_reader->GetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &native_type);
        if (FAILED(hr)) return hr;

        UINT32 w = 0, h = 0;
        ::MFGetAttributeSize(native_type.Get(), MF_MT_FRAME_SIZE, &w, &h);
        scale(w, h, max_side_len);

        // create clean uncompressed media type
        ComPtr<::IMFMediaType> target_type;
        ::MFCreateMediaType(&target_type);
        target_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        target_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        ::MFSetAttributeSize(target_type.Get(), MF_MT_FRAME_SIZE, w, h);

        hr = safe_reader->SetCurrentMediaType(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, target_type.Get());
        if (FAILED(hr)) return hr;

        safe_reader->SetStreamSelection(cast::to_u32(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        safe_reader->SetStreamSelection(cast::to_u32(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);

        final_w = w;
        final_h = h;
        m_reader = std::move(safe_reader);
        return S_OK;
    };

    // try GPU accelerated decoding first
    hr = try_setup_reader(true);
    if (FAILED(hr))
    {
        LogWarn("GPU decode setup failed for '" << filepath << "', falling back to CPU. Err: " << LogHRES(hr));

        // fallback to software CPU decoding
        hr = try_setup_reader(false);
        if (FAILED(hr)) [[unlikely]]
            return log_and_reset_err("Failed to setup reader (CPU fallback) for", hr);
    }

    std::optional<bool> valid;
    m_info = get_info(filepath, max_side_len, valid);
    if (!valid.value_or(false)) [[unlikely]]
        return log_and_reset_err("Failed to get video info for", hr);

    m_info.width = final_w;
    m_info.height = final_h;

    // allocate buffer for IM_COL32(RGBA) format
    m_frame_buffer.reset(new byte[final_h * final_w * 4], [](byte* p) { if (p) delete[] p; });

    m_video_id = ++m_last_video_id;
    m_has_video = true;
    m_is_playing.store(false, std::memory_order_relaxed);
    m_current_time.store(0.0f, std::memory_order_relaxed);
    m_seek_requested.store(false, std::memory_order_relaxed);
    m_frame_ready.store(false, std::memory_order_relaxed);
    m_last_frame_request.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

    seek(m_video_id, 0.0f);
    m_stop_thread = std::stop_source();
    m_decode_thread = std::thread(&VideoControl_Win32::decode_thread_loop, this, m_info);

    Log("Video #" << m_video_id << " loaded from " << filepath);
    return m_video_id;
}

void VideoControl_Win32::play_impl(int32_t video_id) noexcept
{
    LogRAssert(m_video_id == video_id, "Video must be loaded first", );
    if (m_gif_playing)
        return m_gif_player->play();

    if (!m_initialized || !m_has_video || m_is_playing.load(std::memory_order_relaxed))
        return;

    // rewind only if we reached the end of the file
    if (m_current_time.load(std::memory_order_relaxed) >= m_info.duration_sec)
        seek(video_id, 0.0f);

    m_is_playing.store(true, std::memory_order_relaxed);
    Log("Video #" << m_video_id << ": start playing");
    m_cv.notify_all();
}

void VideoControl_Win32::stop_impl(int32_t video_id) noexcept
{
    LogRAssert(m_video_id == video_id, "Video must be loaded first", );
    m_video_id = 0;
    if (m_gif_playing)
        return m_gif_player->stop();

    if (!m_initialized || !m_has_video)
        return;

    m_is_playing.store(false, std::memory_order_relaxed);
    Log("Video #" << video_id << ": stop playing");
}

void VideoControl_Win32::seek(int32_t video_id, float pos) noexcept
{
    LogRAssert(m_video_id == video_id, "Video must be loaded first", );
    if (m_gif_playing)
        return m_gif_player->seek(pos);

    if (!m_initialized || !m_has_video)
        return;

    if (pos > m_info.duration_sec) pos = m_info.duration_sec;
    else if (pos < 0.0f) pos = 0.0f;

    m_seek_target.store(pos, std::memory_order_relaxed);
    m_seek_requested.store(true, std::memory_order_relaxed);
    Log("Video #" << m_video_id << ": seek to " << pos);
    m_cv.notify_all();
}

bool VideoControl_Win32::get_is_playing(int32_t video_id) const noexcept
{
    if (video_id != m_video_id)
        return false;

    if (m_gif_playing)
        return m_gif_player->get_is_playing();

    if (!m_initialized || !m_has_video)
        return false;
    return m_is_playing.load(std::memory_order_relaxed);
}

float VideoControl_Win32::get_position(int32_t video_id) const noexcept
{
    if (video_id != m_video_id)
        return 0.0f;

    if (m_gif_playing)
        return m_gif_player->get_position();

    if (!m_initialized || !m_has_video)
        return 0.0f;

    return m_current_time.load(std::memory_order_relaxed);
}

ImTextureID VideoControl_Win32::get_frame_impl(int32_t video_id, float* out_w /*= nullptr*/, float* out_h /*= nullptr*/)
{
    LogRAssert(m_video_id == video_id, "Video must be loaded first", nullptr);
    if (m_gif_playing)
        return m_gif_player->get_frame(out_w, out_h);

    if (!m_initialized || !m_has_video)
        return nullptr;

    m_last_frame_request.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
    if (m_frame_ready.load(std::memory_order_relaxed))
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_texture) // first frame
        {
            m_texture = g::pTextures->upload_to_gpu(m_frame_buffer.get(), m_info.width, m_info.height, true);
        }
        else // next frames
        {
            g::pTextures->update_gpu_texture(m_texture, m_frame_buffer.get(), m_info.width, m_info.height, true);
        }

        m_frame_ready.store(false, std::memory_order_relaxed);
    }

    if (out_w) *out_w = cast::to_float(m_info.width);
    if (out_h) *out_h = cast::to_float(m_info.height);
    return m_texture.get(); // returns updated frame or previous frame or null
}

void VideoControl_Win32::decode_thread_loop(mngr::VideoInfo info)
{
    struct ComInitRAII 
    {
        HRESULT hr = S_OK;
        ComInitRAII() { hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
        ~ComInitRAII() { if (SUCCEEDED(hr)) ::CoUninitialize(); }
    }
    com_init;

    const int32_t frame_ms = info.fps > 0.0f ? cast::to_i32(1000.0f / info.fps) : 33;

    while (!m_stop_thread.stop_requested())
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
            m_cv.wait(lock, [this]() noexcept
                {
                    return
                        m_is_playing.load(std::memory_order_relaxed) || 
                        m_seek_requested.load(std::memory_order_relaxed) || 
                        m_stop_thread.stop_requested();
                });
            if (m_stop_thread.stop_requested())
                break;
            continue;
        }

        if (m_seek_requested.load(std::memory_order_relaxed))
        {
            const float target = m_seek_target.load(std::memory_order_relaxed);
            ::PROPVARIANT var;
            ::PropVariantInit(&var);
            const HRESULT hr = ::InitPropVariantFromInt64(cast::to_i64(target * 10000000.0f), &var);
            if (SUCCEEDED(hr))
            {
                m_reader->SetCurrentPosition(GUID_NULL, var);
                ::PropVariantClear(&var);
            }        
            m_seek_requested.store(false, std::memory_order_relaxed);
            m_frame_ready.store(false, std::memory_order_relaxed);    
        }

        const auto frame_start = std::chrono::steady_clock::now();

        if (m_is_playing.load(std::memory_order_relaxed) || !m_frame_ready.load(std::memory_order_relaxed))
        {
            ComPtr<::IMFSample> sample;
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
                ComPtr<::IMFMediaBuffer> buffer;
                hr = sample->ConvertToContiguousBuffer(&buffer);
                if (SUCCEEDED(hr))
                {
                    ComPtr<::IMF2DBuffer> buffer2d;
                    BYTE* scanline0 = nullptr;
                    LONG pitch = 0;
                    BYTE* data1d = nullptr;
                    DWORD length1d = 0;
                    bool is_2d = false;

                    // Try to get 2D buffer interface for proper stride/pitch handling
                    if (SUCCEEDED(buffer.As(&buffer2d)))
                    {
                        hr = buffer2d->Lock2D(&scanline0, &pitch);
                        is_2d = SUCCEEDED(hr);
                    }

                    // Fallback to standard 1D lock if 2D is not supported by this format
                    if (!is_2d)
                    {
                        hr = buffer->Lock(&data1d, nullptr, &length1d);
                        scanline0 = data1d;
                        pitch = info.width * 4; // Assume tightly packed RGB32
                    }

                    if (SUCCEEDED(hr))
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        uint32_t* dst_buffer = cast::ptrs<uint32_t>(m_frame_buffer.get());

                        // Copy line by line using the reported pitch
                        for (int32_t y = 0; y < info.height; ++y)
                        {
                            // scanline0 is BYTE*, so we offset by bytes (pitch) and then cast to uint32_t*
                            const uint32_t* src_line = cast::ptrs<uint32_t>(scanline0 + (y * pitch));
                            uint32_t* dst_line = dst_buffer + (y * info.width);

                            for (int32_t x = 0; x < info.width; ++x)
                            {
                                const uint32_t pixel = src_line[x];
                                // Swizzle BGRA to IM_COL32(RGBA) format
                                dst_line[x] = 0xFF000000 | (pixel & 0x0000FF00) | ((pixel & 0x00FF0000) >> 16) | ((pixel & 0x000000FF) << 16);
                            }
                        }

                        if (is_2d)
                            buffer2d->Unlock2D();
                        else
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
                m_cv.wait_for(lock, time_to_sleep, [this]() noexcept
                    {
                        return
                            m_seek_requested.load(std::memory_order_relaxed) || 
                            !m_is_playing.load(std::memory_order_relaxed) ||
                            m_stop_thread.stop_requested();
                    });
            }
        }
    }
}

bool VideoControl_Win32::get_thumbnail_check_window_cache(const std::filesystem::path& filename, const std::string& dest_filename, uint32_t max_side_len)
{
    ComPtr<::IShellItemImageFactory> factory;
    if (SUCCEEDED(::SHCreateItemFromParsingName(filename.c_str(), nullptr, IID_PPV_ARGS(&factory))))
    {
        HBITMAP bitmap = nullptr;
        const SIZE size = { cast::to_i32(max_side_len), cast::to_i32(max_side_len) };

        if (SUCCEEDED(factory->GetImage(size, SIIGBF_RESIZETOFIT, &bitmap)))
        {
            std::unique_ptr<HBITMAP__, decltype(&::DeleteObject)> hbitmap(bitmap, ::DeleteObject);

            ::BITMAP bmp;
            if (::GetObjectW(hbitmap.get(), sizeof(BITMAP), &bmp))
            {
                const int32_t w = bmp.bmWidth;
                const int32_t h = bmp.bmHeight;

                ::BITMAPINFOHEADER bi = {};
                bi.biSize = sizeof(::BITMAPINFOHEADER);
                bi.biWidth = w;
                bi.biHeight = -h; // negative height for top-down DIB
                bi.biPlanes = 1;
                bi.biBitCount = 32;
                bi.biCompression = BI_RGB;

                std::unique_ptr<HDC__, void(*)(::HDC)> hdc(::GetDC(nullptr), [](::HDC hdc) { if (hdc) ::ReleaseDC(nullptr, hdc); });
                std::vector<byte> pixels(w * h * 4);

                if (::GetDIBits(hdc.get(), hbitmap.get(), 0, h, pixels.data(), cast::ptrs<::BITMAPINFO>(&bi), DIB_RGB_COLORS))
                {
                    const size_t num_pixels = w * h;
                    uint32_t* ptr = cast::ptrs<uint32_t>(pixels.data());

                    // swizzle BGRA to RGBA with forced opaque alpha
                    for (size_t i = 0; i < num_pixels; ++i)
                    {
                        const uint32_t p = ptr[i];
                        ptr[i] = 0xFF000000 | (p & 0x0000FF00) | ((p & 0x00FF0000) >> 16) | ((p & 0x000000FF) << 16);
                    }

                    const auto write_res = stbi_write_png(dest_filename.c_str(), w, h, 4, pixels.data(), w * 4);
                    if (write_res != 0 && tools::path::exists(dest_filename))
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

} // namespace platform

#endif // _WIN32
