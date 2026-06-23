#pragma once
#include "miniaudio.h"
#include "core/base/structs.h"

namespace global
{

class AudioControl final
{
public:
    AudioControl() = default;
    ~AudioControl();

    NO_COPY_MOVE(AudioControl);

    void init();
    void cleanup_all();

    mngr::AudioInfo get_info(const std::string& filepath, std::optional<bool>& valid);

    int32_t load(const std::string& filepath);
    void play(int32_t audio_id);
    void stop(int32_t audio_id);
    void seek(int32_t audio_id, float pos);

    int32_t get_current() const { return m_audio_id; }
    bool get_is_playing(int32_t audio_id) const { return m_audio_id == audio_id && m_is_playing.load(std::memory_order_relaxed); }
    float get_position(int32_t audio_id) const;

private:
    void cleanup();
    static void end_callback(void* user_data, ma_sound* sound);

private:
    ma_engine m_engine;
    ma_sound m_sound;

    int32_t m_last_media_id = 0;
    int32_t m_audio_id = 0;
    bool m_initialized = false;
    bool m_has_sound = false;

    // Using atomic because the EOF callback fires from the audio thread
    std::atomic_bool m_is_playing = false;

    float m_duration_sec = 0.0f;
    uint32_t m_sample_rate = 0;
};

} // namespace global
