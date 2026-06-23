#include "ui/helpers/audio_control.h"

namespace global
{

AudioControl::~AudioControl()
{
    cleanup_all();
}

void AudioControl::init()
{
    const auto res = ma_engine_init(NULL, &m_engine);
    if (res != MA_SUCCESS)
    {
        LogWarn("Failed to initialize audio engine, err " << res);
        return;
    }

    m_initialized = true;
}

void AudioControl::cleanup_all()
{
    if (m_initialized)
    {
        cleanup();
        ma_engine_uninit(&m_engine);
        m_initialized = false;
    }
}

void AudioControl::cleanup()
{
    if (m_has_sound)
    {
        ma_sound_stop(&m_sound);
        ma_sound_uninit(&m_sound);

        m_has_sound = false;
        m_is_playing.store(false, std::memory_order_relaxed);
        m_audio_id = 0;
    }
}

mngr::AudioInfo AudioControl::get_info(const std::string& filepath, std::optional<bool>& valid)
{
    if (!m_initialized)
        return {};

    ma_decoder decoder;

    // Initialize the decoder to read the file header
    ma_result result = ma_decoder_init_file(filepath.c_str(), NULL, &decoder);
    if (result != MA_SUCCESS)
    {
        valid = false;
        return {};
    }

    mngr::AudioInfo info;

    // Extract length in PCM frames
    ma_uint64 length_pcm = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &length_pcm) == MA_SUCCESS)
    {
        // Calculate duration in seconds
        info.duration_sec = static_cast<float>(length_pcm) / decoder.outputSampleRate;
    }

    // Extract format details directly from the decoder structure
    info.sample_rate = decoder.outputSampleRate;
    info.channels = decoder.outputChannels;
    valid = true;

    // Clean up
    ma_decoder_uninit(&decoder);

    return info;
}

void AudioControl::end_callback(void* user_data, ma_sound* /*sound*/)
{
    // this executes in miniaudio's background thread

    const auto control = cast::ptrs<AudioControl>(user_data);
    if (control != nullptr)
    {
        control->m_is_playing.store(false, std::memory_order_relaxed);
    }
}

int32_t AudioControl::load(const std::string& filepath)
{
    if (!m_initialized)
        return 0;

    cleanup();

    // initialize sound without starting it
    ma_result result = ma_sound_init_from_file(&m_engine, filepath.c_str(), 0, nullptr, nullptr, &m_sound);
    if (result != MA_SUCCESS)
    {
        Log("Failed to init audio '" << filepath << "', err " << result);
        return 0;
    }

    // EOF callback
    result = ma_sound_set_end_callback(&m_sound, end_callback, this);
    if (result != MA_SUCCESS)
    {
        Log("Failed to init audio '" << filepath << "', err " << result);
        return 0;
    }

    // get track length and sample rate for the timeline
    ma_uint64 length_pcm = 0;
    result = ma_sound_get_length_in_pcm_frames(&m_sound, &length_pcm);
    if (result != MA_SUCCESS)
    {
        Log("Failed to init audio '" << filepath << "', err " << result);
        return 0;
    }

    ma_format format;
    uint32_t channels;
    result = ma_sound_get_data_format(&m_sound, &format, &channels, &m_sample_rate, nullptr, 0);
    if (result != MA_SUCCESS)
    {
        Log("Failed to init audio '" << filepath << "', err " << result);
        return 0;
    }

    //ma_sound_seek_to_pcm_frame(&m_sound, 0);

    m_audio_id = ++m_last_media_id;
    m_duration_sec = cast::to_float(length_pcm) / cast::to_float(m_sample_rate);
    m_has_sound = true;
    m_is_playing.store(false, std::memory_order_relaxed);
    Log("Loaded audio '" << filepath << "', id=" << m_audio_id);
    return m_audio_id;
}

void AudioControl::play(int32_t audio_id)
{
    if (m_audio_id != audio_id)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Audio must be loaded first");
    }

    if (!m_initialized || !m_has_sound || m_is_playing.load(std::memory_order_relaxed))
        return;

    // rewind only if we reached the end of the file
    if (ma_sound_at_end(&m_sound))
        ma_sound_seek_to_pcm_frame(&m_sound, 0);

    ma_sound_start(&m_sound);
    m_is_playing.store(true, std::memory_order_relaxed);
}

void AudioControl::stop(int32_t audio_id)
{
    if (m_audio_id != audio_id)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Audio must be loaded first");
    }

    if (!m_initialized || !m_has_sound || !m_is_playing.load(std::memory_order_relaxed))
        return;

    ma_sound_stop(&m_sound);
    m_is_playing.store(false, std::memory_order_relaxed);
}

void AudioControl::seek(int32_t audio_id, float pos)
{
    if (m_audio_id != audio_id)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Audio must be loaded first");
    }

    if (!m_initialized || !m_has_sound)
        return;

    // ignore m_is_playing flag and allow to seek for stopped audio

    const ma_uint64 target_pcm = static_cast<ma_uint64>(pos * m_sample_rate);
    const ma_uint64 total_pcm = static_cast<ma_uint64>(m_duration_sec * m_sample_rate);
    ma_sound_seek_to_pcm_frame(&m_sound, target_pcm);
}

float AudioControl::get_position(int32_t audio_id) const
{
    if (!m_initialized || !m_has_sound || m_audio_id != audio_id)
        return 0.0f;

    ma_uint64 cursor_pcm = 0;
    ma_sound_get_cursor_in_pcm_frames(&m_sound, &cursor_pcm);
    return cast::to_float(cursor_pcm) / cast::to_float(m_sample_rate);
}

} // namespace global
