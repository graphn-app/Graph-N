#pragma once
#include "core/listeners.h"

namespace global
{

class MainRoutine;
struct ImageAsync;

class Textures : public listener::Timer
{
public:
    Textures();
    virtual ~Textures() noexcept override;

    void init_ui_ready();

    int32_t load_image_from_path(const std::filesystem::path& path, int32_t max_width, int32_t max_height, int32_t life_time);
    int32_t load_image_from_buffer(std::string&& buffer, int32_t max_width, int32_t max_height, int32_t life_time);

    // must be called from main UI thread
    ImTextureID get_image(int32_t image_id, std::optional<bool>& image_valid, float* width = nullptr, float* height = nullptr);
    bool get_image_alive(int32_t image_id);
    void get_original_image_size(int32_t image_id, int32_t& width, int32_t& height);

    void release_image(int32_t image_id);

    // Lazy1sTimer
    void on_1s_lazy_timer(int64_t total_seconds_elapsed) override;
    void on_frame(int64_t total_frames_elapsed) noexcept override { m_frames_passed = total_frames_elapsed; }

    void cleanup_all();

    virtual ImTextureIDPtr upload_to_gpu(const byte* image_data, uint32_t width, uint32_t height, bool generate_mips) = 0;
    virtual bool update_gpu_texture(ImTextureIDPtr texture, const byte* image_data, uint32_t width, uint32_t height, bool generate_mips) = 0;

private:
    int32_t load_image(const std::filesystem::path& path, std::string&& buffer, int32_t max_width, int32_t max_height, int32_t life_time);
    void worker_thread();
    bool check_image_lifecycle(const ImageAsync* image) noexcept;

private:
    std::mutex m_mutex;
    int32_t m_id_counter = 0;
    size_t m_seconds_passed = 0;
    int64_t m_frames_passed = 0;

    std::vector<std::thread> m_workers;
    std::condition_variable m_cv;

    std::queue<std::shared_ptr<ImageAsync>> m_waiting;
    std::map<int32_t, std::shared_ptr<ImageAsync>> m_images;
};

} // namespace global
