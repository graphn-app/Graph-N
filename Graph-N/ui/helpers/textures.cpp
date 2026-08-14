#include "ui/helpers/textures.h"
#include "stb_image_resize2.h"

#ifndef STBIR_FREE
#define STBIR_FREE(ptr,user_data)    ((void)(user_data), free(ptr))
#endif

namespace global
{

struct ImageAsync final
{
    int32_t image_id = 0;
    ImTextureIDPtr textures;
    std::string path;
    std::string buffer;
    ImageData image_data;

    int32_t width = 0;
    int32_t height = 0;
    int32_t width_or = 0;
    int32_t height_or = 0;
    bool generate_mips = false;
    size_t life_time = 0;
    size_t last_access_time = 0;

    int64_t frame_requested = 0;
    bool ever_requested = false;

    std::atomic_bool image_data_ready = false;
    std::atomic_bool load_failed = false;
    std::atomic_bool force_release_image = false;
};

Textures::Textures()
{
    // determine optimal thread count based on hardware, minimum 4
    const uint32_t num_threads = std::max(4u, std::thread::hardware_concurrency());

    for (uint32_t i = 0; i < num_threads; ++i)
    {
        m_workers.emplace_back(&Textures::worker_thread, this);
    }
}

Textures::~Textures() noexcept
{
    try
    {
        if (g::pGraphN != nullptr)
            g::pGraphN->unsubscribe_timers(this);

        cleanup_all();
    }
    catch (...) {}
}

void Textures::init_ui_ready()
{
    g::pGraphN->subscribe_timers(this);
}

int32_t Textures::load_image(const char* path, std::string&& buffer, int32_t max_width, int32_t max_height, int32_t life_time)
{
    if (g::app_global_stop_all_threads.stop_requested())
        return -1;

    if (buffer.empty() && (nullptr == path || !tools::path::exists(path))) [[unlikely]]
    {
        LogRError("Can't load image: no input data", -1);
    }

    std::shared_ptr<ImageAsync> image = std::make_shared<ImageAsync>();
    image->image_id = ++m_id_counter;
    image->path = path;
    image->buffer = std::move(buffer);
    image->width = max_width;
    image->height = max_height;
    image->life_time = life_time;
    image->last_access_time = m_seconds_passed;
    image->frame_requested = m_frames_passed;

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_images[image->image_id] = image;
        m_waiting.push(image);
    }
    m_cv.notify_one();

    Log("Image queued. ID: " << image->image_id << ". Path (" << path << "), buffer: " << image->buffer.size()
        << ". w/h: " << max_width << "/" << max_height << ", lt: " << life_time);

    return image->image_id;
}

int32_t Textures::load_image(const char* path, int32_t max_width, int32_t max_height, int32_t life_time)
{
    return load_image(path, {}, max_width, max_height, life_time);
}

int32_t Textures::load_image(std::string&& buffer, int32_t max_width, int32_t max_height, int32_t life_time)
{
    return load_image("", std::move(buffer), max_width, max_height, life_time);
}

void Textures::worker_thread()
{
    while (!g::app_global_stop_all_threads.stop_requested())
    {
        std::shared_ptr<ImageAsync> image;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() noexcept {
                return !m_waiting.empty() || g::app_global_stop_all_threads.stop_requested();
            });

            if (g::app_global_stop_all_threads.stop_requested() && m_waiting.empty())
                break;

            image = m_waiting.front();
            m_waiting.pop();
        }

        if (image->force_release_image.load(std::memory_order_relaxed))
        {
            image->load_failed.store(true, std::memory_order_release);
            image->image_data_ready.store(true, std::memory_order_release);
            Log("Image early exit. ID: " << image->image_id);
            continue;
        }

        Log("Image processing started. ID: " << image->image_id);

        ImageData image_data(nullptr, [](stbi_uc* p) { stbi_image_free(p); });

        try
        {
            if (image->buffer.empty())
                image_data.reset(stbi_load(image->path.c_str(), &image->width_or, &image->height_or, nullptr, 4));
            else
                image_data.reset(stbi_load_from_memory((stbi_uc*)image->buffer.data(), cast::to_i32(image->buffer.size()), &image->width_or, &image->height_or, nullptr, 4));

            if (!image_data || image->width_or <= 0 || image->height_or <= 0 || image->width_or > 16384 || image->height_or > 16384)
            {
                Log("Image load failed. ID: " << image->image_id << ". w=" << image->width_or << ", h=" << image->height_or);
                image->load_failed.store(true, std::memory_order_release);
            }
            else [[likely]]
            {
                int32_t final_w, final_h;
                tools::img::scale(image->width_or, image->height_or, image->width, image->height, final_w, final_h);

                if (final_w != image->width_or || final_h != image->height_or)
                {
                    ImageData image_data_resized(nullptr, [](unsigned char* p) { STBIR_FREE(p, nullptr); });
                    image_data_resized.reset(stbir_resize_uint8_linear(image_data.get(), image->width_or, image->height_or, 0, nullptr, final_w, final_h, 0, STBIR_RGBA));

                    if (!image_data_resized) [[unlikely]]
                    {
                        Log("Image resize failed. ID: " << image->image_id);
                        image->load_failed.store(true, std::memory_order_release);
                    }
                    else
                    {
                        image->width = final_w;
                        image->height = final_h;
                        image->image_data = image_data_resized;
                        image->generate_mips = true;
                    }
                }
                else
                {
                    image->width = image->width_or;
                    image->height = image->height_or;
                    image->image_data = image_data;
                }
            }
        }
        catch (const std::exception& ex)
        {
            LogExcep("Exception in texture load/resize for ID: " << image->image_id << ". " << ex.what());
            image->load_failed.store(true, std::memory_order_release);
        }
        catch (...)
        {
            LogExcep("Exception in texture load/resize for ID: " << image->image_id);
            image->load_failed.store(true, std::memory_order_release);
        }

        image->buffer.clear(); image->buffer.shrink_to_fit();
        image->path.clear(); image->path.shrink_to_fit();

        Log("Image processing finished. ID: " << image->image_id);

        image->image_data_ready.store(true, std::memory_order_release);
    }
}

ImTextureID Textures::get_image(int32_t image_id, std::optional<bool>& image_valid, float* width /*= nullptr*/, float* height /*= nullptr*/)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_images.find(image_id);
    if (it == m_images.end())
        return nullptr;

    auto image = it->second.get();
    if (!image)
        return nullptr;

    image->ever_requested = true;
    if (image->image_data_ready.load(std::memory_order_acquire))
    {
        if (image->load_failed.load(std::memory_order_acquire)) [[unlikely]]
        {
            m_images.erase(it);
            image_valid = false;
            return nullptr;
        }

        if (image->force_release_image.load(std::memory_order_acquire)) [[unlikely]]
        {
            m_images.erase(it);
            return nullptr;
        }

        if (width != nullptr)   *width = cast::to_float(image->width);
        if (height != nullptr)  *height = cast::to_float(image->height);

        image_valid = true;
        image->last_access_time = m_seconds_passed;

        if (image->textures) [[likely]]
            return image->textures.get();

        try
        {
            image->textures = upload_to_gpu(image->image_data, image->width, image->height, image->generate_mips);
        }
        catch (const std::exception& ex)
        {
            LogExcep("Exception during processing image ID: " << image->image_id << ". " << ex.what());
            m_images.erase(it);
            return nullptr;
        }
        catch (...)
        {
            LogExcep("Exception during processing image ID: " << image->image_id);
            m_images.erase(it);
            return nullptr;
        }
        image->image_data = {};

        if (image->textures)
            return image->textures.get();

        Log("Image upload failed. ID: " << image->image_id);
        m_images.erase(it);
    }

    return nullptr;
}

bool Textures::get_image_alive(int32_t image_id)
{
    if (0 == image_id) return false;
    std::unique_lock<std::mutex> lock(m_mutex);
    return (m_images.find(image_id) != m_images.end());
}

void Textures::get_original_image_size(int32_t image_id, int32_t& width, int32_t& height)
{
    LogRAssert(image_id > 0, "Invalid image id", );
    std::unique_lock<std::mutex> lock(m_mutex);
    const auto it = m_images.find(image_id);
    if (it == m_images.end())
        return;
    auto& image = it->second;
    width = image->width_or;
    height = image->height_or;
}

void Textures::release_image(int32_t image_id)
{
    LogRAssert(image_id > 0, "Invalid image id", );

    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_images.find(image_id);
    if (it == m_images.end())
        return;

    auto image = it->second.get();
    if (image)
    {
        if (image->image_data_ready.load(std::memory_order_acquire))
        {
            image->textures = {};
            m_images.erase(it);
        }
        else
        {
            image->force_release_image.store(true, std::memory_order_release);
        }
    }
}

bool Textures::check_image_lifecycle(const ImageAsync* image) noexcept
{
    LogRAssert(image, "Zero ptr", false);

    if (image->image_data_ready.load(std::memory_order_acquire) && image->life_time > 0)
    {
        if (m_seconds_passed - image->last_access_time > image->life_time) [[unlikely]]
        {
            Log("Image dead. ID: " << image->image_id);
            return false;
        }
    }
    return true;
}

void Textures::on_1s_lazy_timer(int64_t total_seconds_elapsed)
{
    m_seconds_passed = total_seconds_elapsed;
    if (m_seconds_passed % 5 == 0)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        for (auto it = m_images.begin(); it != m_images.end(); )
        {
            const auto* image = it->second.get();
            if (!image)
            {
                it = m_images.erase(it);
                continue;
            }

            if (!image->ever_requested && m_frames_passed - image->frame_requested > 60) [[unlikely]]
            {
                Log("Image early exit. ID: " << image->image_id);
                it = m_images.erase(it);
            }
            else if (image->image_data_ready.load(std::memory_order_acquire))
            {
                if (image->load_failed.load(std::memory_order_acquire) || image->force_release_image.load(std::memory_order_acquire))
                {
                    it = m_images.erase(it);
                }
                else if (!check_image_lifecycle(image))
                {
                    it = m_images.erase(it);
                }
                else [[likely]]
                {
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }
    }
}

void Textures::cleanup_all()
{
    m_cv.notify_all();

    for (auto& worker : m_workers)
    {
        if (worker.joinable())
            worker.join();
    }
    m_workers.clear(); m_workers.shrink_to_fit();

    std::unique_lock<std::mutex> lock(m_mutex);
    std::queue<std::shared_ptr<ImageAsync>> empty_queue;
    std::swap(m_waiting, empty_queue);
    m_images.clear();
}

} // namespace global
