#pragma once
#include "core/listeners.h"

namespace platform
{

class MainRoutine;
struct ImageAsync;

using ReleaseTexturesCallback = std::function<void(ImTextureID)>;
using ImTextureIDPtr = std::shared_ptr<std::remove_pointer_t<ImTextureID>>;
using ImageData = std::shared_ptr<unsigned char>;

class Textures : public listener::Timer
{
public:
    Textures();
    virtual ~Textures();

    NO_COPY_MOVE(Textures);

    void init_ui_ready();
    virtual void set_main_routine(MainRoutine*) {}

    int32_t load_image(const char* path, int32_t max_width, int32_t max_height, int32_t life_time);
    int32_t load_image(std::vector<byte>&& buffer, int32_t max_width, int32_t max_height, int32_t life_time);
    ImTextureID get_image(int32_t image_id, float& width, float& height);
    bool get_image_alive(int32_t image_id);
    void release_image(int32_t image_id);

    // Lazy1sTimer
    void on_1s_lazy_timer(int64_t total_seconds_elapsed) override;
    void on_frame(int64_t /*total_frames_elapsed*/) override {}

    void cleanup_all();

protected:
    virtual ImTextureIDPtr upload_to_gpu(const ImageData& image_data, uint32_t width, uint32_t height, bool generate_mips) = 0;

private:
    int32_t load_image(const char* path, std::vector<byte>&& buffer, int32_t max_width, int32_t max_height, int32_t life_time);
    void check_queue();
    void create_loader(std::shared_ptr<ImageAsync> image);
    bool check_image_data_ready(ImageAsync* image);
    bool check_image_lifecycle(ImageAsync* image);

private:
    const size_t m_max_threads;
    std::atomic_int32_t m_active_threads = 0;
    size_t m_seconds_passed = 0;
    std::mutex m_mutex;
    int32_t m_id_counter = 0;
    std::queue<std::shared_ptr<ImageAsync>> m_waiting;
    std::map<int32_t, std::shared_ptr<ImageAsync>> m_images;
};

} // namespace platform


