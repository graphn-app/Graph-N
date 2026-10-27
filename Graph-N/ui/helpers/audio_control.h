/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/listeners.h"
#include "miniaudio.h"

namespace global
{

class AudioControl final : public listener::Timer
{
public:
    AudioControl() noexcept = default;
    ~AudioControl() noexcept override;

    void init();
    void stop_and_cleanup();

    mngr::AudioInfo get_info(const std::string& filepath, std::optional<bool>& valid);
    int32_t load(const std::string& filepath);

    void play(int32_t audio_id);
    void stop(int32_t audio_id);
    void seek(int32_t audio_id, float pos);

    int32_t get_current() const noexcept { return m_audio_id; }
    bool get_is_playing(int32_t audio_id) noexcept;
    float get_position(int32_t audio_id) const;

private:
    void cleanup_current();
    static void end_callback(void* user_data, ma_sound* sound) noexcept;

    // Timer
    void on_1s_lazy_timer(int64_t total_seconds_elapsed) override;

private:
    ma_engine m_engine = {};
    ma_sound m_sound = {};

    int32_t m_last_media_id = 0;
    int32_t m_audio_id = 0;
    bool m_initialized = false;
    bool m_has_sound = false;

    // Using atomic because the EOF callback fires from the audio thread
    std::atomic_bool m_is_playing = false;

    float m_duration_sec = 0.0f;
    uint32_t m_sample_rate = 0;

    double m_last_requested = 0.0;
};

} // namespace global
