
#include "ui/textures.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

namespace platform
{

struct ImageAsync
{
    int32_t image_id = 0;
    ImTextureIDPtr textures;
    std::string path;
    std::vector<byte> buffer;
    ImageData image_data;
    std::atomic_bool image_data_ready = false;
    int32_t width = 0;
    int32_t height = 0;
    bool generate_mips = false;
    size_t life_time = 0;
    size_t last_access_time = 0;
    std::future<void> future;
    bool force_release_image = false;
};

Textures::Textures()
    : m_max_threads(20)
{
}

Textures::~Textures()
{
    LogF;
}

void Textures::init_ui_ready()
{
    g::pGraphN->subscribe_timers(this);
}

int32_t Textures::load_image(const char* path, std::vector<byte>&& buffer, int32_t max_width, int32_t max_height, int32_t life_time) // locks inside
{
    if (buffer.empty() && (nullptr == path || !tools::path::exists(path)))
    {
        LogInternalLogic("Can't load image: no input data");
        return -1;
    }

    bool load_image = false;
    std::shared_ptr<ImageAsync> image = std::make_shared<ImageAsync>();
    image->image_id = ++m_id_counter;
    image->path = path;
    image->buffer = std::move(buffer);
    image->width = max_width;
    image->height = max_height;
    image->life_time = life_time;

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_active_threads >= m_max_threads)
        {
            m_waiting.push(image);
            Log("Image queued. ID: " << image->image_id << ". Path (" << path << "), buffer: " << image->buffer.size()
                << ". w/h: " << max_width << "/" << max_height << ", lt: " << life_time);
        }
        else
        {
            m_images[image->image_id] = image;
            load_image = true;
        }
    }

    if (load_image)
        create_loader(image);

    return image->image_id;
}

void Textures::create_loader(std::shared_ptr<ImageAsync> image) // thread safe
{
    Log("Image started. ID: " << image->image_id << ". Path (" << image->path << "), buffer: " << image->buffer.size()
        << ". w/h: " << image->width << "/" << image->height << ", lt: " << image->life_time);

    m_active_threads++;
    image->future = std::async(std::launch::async, [weak_image = std::weak_ptr<ImageAsync>(image)]()
    {
        auto image = weak_image.lock();
        if (!image)
            return;

        int32_t w = 0, h = 0;
        ImageData image_data(nullptr, [](stbi_uc* p) { stbi_image_free(p); });
        if (image->buffer.empty())
            image_data.reset(stbi_load(image->path.c_str(), &w, &h, nullptr, 4));
        else
            image_data.reset(stbi_load_from_memory(image->buffer.data(), cast::to_i32(image->buffer.size()), &w, &h, nullptr, 4));

        if (!image_data || w <= 0 || h <= 0 || w > 16384 || h > 16384)
        {
            LogS("Image finished load. ID: " << image->image_id << ". stbi failed. w=" << w << ", h=" << h);
            return;
        }

        image->width = w;
        image->height = h;

        if (image->width > 0 && image->width < w || image->height > 0 && image->height < h)
        {
            int32_t final_w = w;
            int32_t final_h = h;

            const float scale_w = (image->width > 0 ? cast::to_float(image->width) / w : 1e10f);
            const float scale_h = (image->height > 0 ? cast::to_float(image->height) / h : 1e10f);
            const float scale = std::min(scale_w, scale_h);

            final_w = cast::to_i32(w * scale);
            final_w = std::max(1, final_w);

            final_h = cast::to_i32(h * scale);
            final_h = std::max(1, final_h);

            ImageData image_data_resized(nullptr, [](unsigned char* p) { STBIR_FREE(p, nullptr); });
            image_data_resized.reset(stbir_resize_uint8_linear(image_data.get(), w, h, 0, nullptr, final_w, final_h, 0, STBIR_RGBA));
            if (!image_data_resized)
            {
                LogS("Image finished resize. ID: " << image->image_id << ". stbi_resize failed");
                return;
            }

            image->width = final_w;
            image->height = final_h;

            image->image_data = image_data_resized;
            image->generate_mips = true;
        }
        else
        {
            image->image_data = image_data;
        }

        LogS("Image finished. ID: " << image->image_id);
    });
}

int32_t Textures::load_image(const char* path, int32_t max_width, int32_t max_height, int32_t life_time)
{
    return load_image(path, {}, max_width, max_height, life_time);
}

int32_t Textures::load_image(std::vector<byte>&& buffer, int32_t max_width, int32_t max_height, int32_t life_time)
{
    return load_image("", std::move(buffer), max_width, max_height, life_time);
}

void Textures::check_queue() // must be locked by called
{
    while (m_active_threads < m_max_threads && !m_waiting.empty())
    {
        auto image = m_waiting.front();
        m_waiting.pop();
        m_images[image->image_id] = image;
        create_loader(image);
    }
}

// if returns false: image must be removed and waiting queue to be checked
bool Textures::check_image_data_ready(ImageAsync* image) // thread safe
{  
    if (!image->image_data_ready)
    {
        if (!image->future.valid())
        {
            LogWarn("Invalid future. ID: " << image->image_id);
            return false;
        }
        if (image->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            m_active_threads--;
            image->future.get();
            if (!image->image_data)
                return false;

            image->last_access_time = m_seconds_passed;
            image->buffer = {};
            image->path = {};
            image->future = {};
            image->image_data_ready = true;
            check_queue();
            return !image->force_release_image;
        }
    }
    return true;
}

// if returns false: image must be removed and waiting queue to be checked
bool Textures::check_image_lifecycle(ImageAsync* image) // thread safe
{
    if (image->image_data_ready && image->life_time > 0)
    {
        if (m_seconds_passed - image->last_access_time > image->life_time)
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
        bool do_check_queue = false;
        std::unique_lock<std::mutex> lock(m_mutex);
        for (auto it = m_images.begin(); it != m_images.end(); )
        {
            auto image = it->second.get();
            if (!check_image_data_ready(image))
            {
                do_check_queue = true;
                it = m_images.erase(it);
            }
            else if (!check_image_lifecycle(image))
            {
                do_check_queue = true;
                it = m_images.erase(it);
            }
            else
                ++it;
        }
        if (do_check_queue)
            check_queue();
    }
}

ImTextureID Textures::get_image(int32_t image_id, float& width, float& height)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_images.find(image_id);
    if (it == m_images.end())
    {
        LogWarn("Image ID: " << image_id << " not found");
        return nullptr;
    }

    auto image = it->second.get();
    if (image->image_data_ready)
    {
        width = cast::to_float(image->width);
        height = cast::to_float(image->height);
        image->last_access_time = m_seconds_passed;

        if (image->textures)
            return image->textures.get();

        image->textures = upload_to_gpu(image->image_data, image->width, image->height, image->generate_mips);
        image->image_data = {};
        if (image->textures)
            return image->textures.get();

        Log("Image upload. ID: " << image->image_id << ". Failed");
        m_images.erase(it);
    }
    else if (!check_image_data_ready(image))
    {
        m_images.erase(it);
        check_queue();
    }

    return nullptr;
}

bool Textures::get_image_alive(int32_t image_id)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto it = m_images.find(image_id);
    return (it != m_images.end());
}

void Textures::release_image(int32_t image_id)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_images.find(image_id);
    if (it == m_images.end())
        return;

    auto image = it->second.get();
    if (image->image_data_ready)
    {
        image->textures = {};
        m_images.erase(it);
    }
    else
    {
        image->force_release_image = true;
    }
}

void Textures::cleanup_all()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_waiting = {};
    for (auto& image : m_images)
    {
        if (!image.second->image_data_ready)
        {
            if (image.second->future.valid())
            {
                image.second->future.wait();
                image.second->future.get();
            }
        }
    }
    m_images = {};
}

} // namespace platform
