
#include "core/managers/models_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/base/comfyui_web_paths.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

ModelsManager::~ModelsManager() noexcept = default;

void ModelsManager::free()
{    
    m_model_items.clear(); m_model_items.shrink_to_fit();
    m_model_to_nodes_map.clear(); m_model_to_nodes_map.rehash(0);
    m_service_components_found = false;
}

void ModelsManager::stop_all_threads()
{
    m_stop_flag.request_stop();

    if (m_th.joinable())
        m_th.join();
}

void ModelsManager::load_models()
{
    stop_all_threads();
    free();

    m_stop_flag = std::stop_source();
    m_th = std::thread(&ModelsManager::load_models_th, this);
}

void ModelsManager::load_models_th()
{
    try
    {
        g::pOptsManager->model_starred_preload();
        const std::string models_req = comfyui_http::models;
        std::string data;
        int32_t http_res = g::pFlow->http_get_and_wait("Models/All", models_req.c_str(), {}, {}, true,
            [stop_flag = m_stop_flag.get_token()]() noexcept { return stop_flag.stop_requested(); }, data);

        if (http_res != HTTP_RESP_OK) [[unlikely]]
        {
            if (m_stop_flag.stop_requested())
                Log("Models thread aborted");
            return;
        }

        std::list<std::string> requests;
        auto j = u_json::parse(data);
        for (const auto& jitem : j)
        {
            if (jitem.contains("name"))                
                requests.push_back(jitem["name"]);
        }

        if (m_stop_flag.stop_requested()) [[unlikely]]
        {
            Log("Models thread aborted");
            return;
        }

        std::vector<std::unique_ptr<data::ModelItem>> models;
        models.reserve(requests.size() * 2);

        for (auto& req : requests)
        {
            std::string model_req = models_req + "/" + req;
            http_res = g::pFlow->http_get_and_wait("Models/Model", model_req.c_str(), {}, {}, false,
                [stop_flag = m_stop_flag.get_token()]() noexcept { return stop_flag.stop_requested(); }, data);

            if (http_res != HTTP_RESP_OK) [[unlikely]]
            {
                if (m_stop_flag.stop_requested())
                {
                    Log("Models thread aborted");
                    return;
                }
                continue;
            }

            j = u_json::parse(data);
            if (j.empty())
            {
                auto root = std::make_unique<data::ModelItem>();
                root->is_model = false;
                root->name = req;
                root->search = tools::str::to_lower_ascii(root->name);
                models.push_back(std::move(root));
            }
            else
            {
                for (const auto& jitem : j)
                {
                    if (jitem.contains("name"))
                    {
                        std::filesystem::path filename = std::string(jitem["name"]);
                        auto item = add_file(req, filename);
                        if (item->name.empty())
                            continue;

                        model_req = comfyui_http::model_meta;
                        model_req += "/" + req;

                        httplib::Params params;
                        params.emplace("filename", tools::str::path_to_string(filename.filename()));
                        http_res = g::pFlow->http_get_and_wait("Models/Model", model_req.c_str(), std::move(params), {}, false,
                            [stop_flag = m_stop_flag.get_token()]() noexcept { return stop_flag.stop_requested(); }, data);

                        if (m_stop_flag.stop_requested()) [[unlikely]]
                        {
                            Log("Models thread aborted");
                            return;
                        }
                        if (HTTP_RESP_OK == http_res)
                            parse_file_info(item.get(), std::move(data));

                        models.push_back(std::move(item));
                    }
                }
            }

            if (m_stop_flag.stop_requested())
            {
                Log("Models thread aborted");
                return;
            }
        }

        rebuild_models(models);

        while (!g::pFlow->get_nodes_ready() && !m_stop_flag.stop_requested())
        {
            std::this_thread::sleep_for(100ms);
        }

        if (m_stop_flag.stop_requested()) [[unlikely]]
        {
            Log("Models thread aborted");
            return;
        }

        sync_to_nodes();
        sort_models();
        setup_model_behaviour();
        calc_number_inside("");

        g::pOptsManager->model_starred_release();
        g::pFlow->set_models_ready();
    }
    catch (const std::exception& ex)
    {
        g::pOptsManager->model_starred_release();
        LogExcep("Models thread ex " << ex.what());
    }
}

std::unique_ptr<data::ModelItem> ModelsManager::add_file(std::string root_folder, std::filesystem::path file_path)
{
    auto root = std::make_unique<data::ModelItem>();
    root->is_model = false;
    root->name = root_folder;
    root->search = tools::str::to_lower_ascii(root->name);

    data::ModelItem* parent = root.get();
    const auto parts = std::distance(file_path.begin(), file_path.end());
    if (parts > 1)
    {
        auto end = file_path.end();
        --end;
        for (auto part = file_path.begin(); part != end; ++part)
        {
            auto fn = std::make_unique<data::ModelItem>();
            fn->is_model = false;
            fn->name = tools::str::path_to_string(*part);
            fn->search = tools::str::to_lower_ascii(fn->name);
            auto ptr = fn.get();
            parent->children.push_back(std::move(fn));
            parent = ptr;
        }
    }

    auto file = std::make_unique<data::ModelItem>();
    file->is_model = true;
    file->name = tools::str::path_to_string(file_path.filename());
    file->search = tools::str::to_lower_ascii(file->name);
    file->hash = base::HashID::make_hash(file->name);
    const auto starred = g::pOptsManager->model_starred_load(file->hash);
    if (starred.has_value())
        file->starred = starred.value();
    parent->children.push_back(std::move(file));

    return root;
}

void ModelsManager::parse_file_info(data::ModelItem* item, std::string&& info)
{
    while (!item->children.empty())
    {
        item = item->children.begin()->get();
    }

    const auto j = o_json::parse(info);
    const auto append_info = [item, &j](const char* key, const char* title)
    {
        if (j.contains(key))
        {
            if (!item->info.info.empty()) item->info.info += "\n";
            item->info.info += title + std::string(j[key]);
        }
    };
    const auto append_info_to = [&j](const char* key, std::string& to)
    {
        if (j.contains(key))
        {
            to = std::string(j[key]);
        }
    };

    if (j.contains("modelspec.title"))
    {
        item->info.info = "Filename: " + item->name;
        item->file_name = item->name;
        item->name = j["modelspec.title"];
    }

    append_info("modelspec.author", "Author: ");
    append_info("modelspec.architecture", "Architecture: ");
    append_info("modelspec.license", "License: ");
    append_info("modelspec.resolution", "Resolution: ");
    append_info("modelspec.language", "Language: ");
    append_info("modelspec.date", "Date: ");
    append_info_to("modelspec.description", item->info.descr);
    append_info_to("modelspec.tags", item->info.tags);
    append_info_to("modelspec.trigger_phrase", item->info.trigger_phrase);

    if (j.contains("modelspec.thumbnail") && j.contains("modelspec.hash_sha256"))
    {
        auto temp = fs::get_temp_directory_path();
        if (tools::path::make_folder(temp))
        {
            auto filename = tools::str::generate_uuid_v4() + ".image";
            const std::string image = j["modelspec.thumbnail"];
            auto pos = image.find(",");
            if (pos != std::string::npos)
            {
                const char* base64 = image.c_str() + ++pos;
                const auto binary = tools::str::base64_decode(base64);
                if (!binary.empty())
                {
                    temp /= filename;
                    if (tools::path::make_file(temp, true, binary))
                    {
                        item->info.image_path = temp;
                    }
                }
            }
        }
    }
}

void ModelsManager::rebuild_models(std::vector<std::unique_ptr<data::ModelItem>>& models)
{
    m_model_items.clear();
    m_model_items.reserve(models.size());

    std::function<void(std::vector<std::unique_ptr<data::ModelItem>>&, std::vector<std::unique_ptr<data::ModelItem>>&)> merge_trees;
    merge_trees = [&merge_trees](std::vector<std::unique_ptr<data::ModelItem>>& src, std::vector<std::unique_ptr<data::ModelItem>>& dst)
    {
        std::unordered_map<std::string, data::ModelItem*> dst_map;
        for (const auto& item : dst)
        {
            dst_map[item->name] = item.get();
        }

        for (auto& src_item : src)
        {
            auto it = dst_map.find(src_item->name);
            data::ModelItem* target_item = nullptr;

            if (it == dst_map.end())
            {
                auto new_item = std::make_unique<data::ModelItem>();
                new_item->is_model = src_item->is_model;
                new_item->starred = src_item->starred;
                new_item->hash = src_item->hash;
                new_item->name = std::move(src_item->name);
                new_item->search = std::move(src_item->search);
                new_item->file_name = std::move(src_item->file_name);
                new_item->info = std::move(src_item->info);

                target_item = new_item.get();
                dst_map[target_item->name] = target_item;
                dst.push_back(std::move(new_item));
            }
            else
            {
                target_item = it->second;
            }

            if (!src_item->children.empty())
            {
                merge_trees(src_item->children, target_item->children);
            }
        }
    };

    merge_trees(models, m_model_items);
    m_model_items.shrink_to_fit();
}

void ModelsManager::sync_to_nodes()
{
    std::function<void(std::vector<std::unique_ptr<data::ModelItem>>& models)> go_models;
    go_models = [this, &go_models](std::vector<std::unique_ptr<data::ModelItem>>& models)
    {
        for (auto& model : models)
        {
            if (model->is_model)
            {
                std::unordered_map<std::string, std::list<hid>>::iterator it;
                if (!model->file_name.empty())
                    it = m_model_to_nodes_map.find(model->file_name);
                else
                    it = m_model_to_nodes_map.find(model->name);
                if (it != m_model_to_nodes_map.end())
                {
                    model->nodes = std::move(it->second);
                    m_model_to_nodes_map.erase(it);
                }
            }
            else
            {
                go_models(model->children);
            }
        }
    };
    go_models(m_model_items);

    m_model_to_nodes_map = {};
}

void ModelsManager::sort_models()
{
    const auto sort_callback = [](const std::unique_ptr<data::ModelItem>& lhs, const std::unique_ptr<data::ModelItem>& rhs) noexcept
    {
        if (!lhs->is_model && rhs->is_model)
            return true;
        if (lhs->is_model && !rhs->is_model)
            return false;

        return lhs->search < rhs->search;
    };

    std::function<void(std::vector<std::unique_ptr<data::ModelItem>>&)> go_sort;
    go_sort = [&go_sort, &sort_callback](std::vector<std::unique_ptr<data::ModelItem>>& models)
    {
        std::ranges::sort(models, sort_callback);
        for (auto& model : models)
        {
            go_sort(model->children);
        }
    };

    go_sort(m_model_items);
}

void ModelsManager::setup_model_behaviour()
{
    std::map<data::ModelItem*, std::list<data::ModelItem*>> root_to_files;

    std::function<void(bool, const std::vector<std::unique_ptr<data::ModelItem>>&, data::ModelItem*)> go_models;
    go_models = [&root_to_files, &go_models](bool is_root, const std::vector<std::unique_ptr<data::ModelItem>>& models, data::ModelItem* root_item)
    {
        data::ModelItem* root = root_item;
        for (auto& model : models)
        {
            if (model->is_model)
            {
                root_to_files[root].push_back(model.get());
            }
            else
            {
                if (is_root)
                {
                    root = model.get();
                    root_to_files[root];
                }
                go_models(false, model->children, root);
            }
        }
    };
    go_models(true, m_model_items, nullptr);

    struct Behaviour final
    {
        const char* node_name = nullptr;
        bool service_component = false;
        bool copy_to_cb_only = false;
    };

    const std::unordered_map<std::string, Behaviour> loaders =
    {
        { "checkpoints",        { "CheckpointLoaderSimple", false, false } },
        { "loras",              { "LoraLoader", false, false } },
        { "vae",                { "VAELoader", false, false } },
        { "text_encoders",      { "CLIPLoader", false, false } },
        { "diffusion_models",   { "UNETLoader", false, false } },
        { "clip_vision",        { "CLIPVisionLoader", false, false } },
        { "style_models",       { "StyleModelLoader", false, false } },
        { "vae_approx",         { "VAEApproxLoader", false, false } },
        { "controlnet",         { "ControlNetLoader", false, false } },
        { "gligen",             { "GLIGENLoader", false, false } },
        { "upscale_models",     { "UpscaleModelLoader", false, false } },
        { "hypernetworks",      { "HypernetworkLoader", false, false } },
        { "photomaker",         { "PhotoMakerLoader", false, false } },
        { "sams",               { "SAMModelLoader", false, false } },
        { "onnx",               { "UltralyticsDetectorProvider", false, false } },
        { "ultralytics_bbox",   { "UltralyticsDetectorProvider", false, false } },
        { "ultralytics_segm",   { "UltralyticsDetectorProvider", false, false } },
        { "unet_gguf",          { "UnetLoaderGGUF", false, false } },
        { "ipadapter",          { "IPAdapterModelLoader", false, false } },
        { "voices",             { "LoadAudio", false, false } },

        { "latent_upscale_models",  { "", true, false } },
        { "model_patches",          { "", true, false } },
        { "inspire_prompts",        { "", true, false } },
        { "VHS_video_formats",      { "", true, false } },

        { "embeddings",             { "", false, true } },
    };

    m_service_components_found = false;

    for (auto& [root, files] : root_to_files)
    {
        if (nullptr == root)
        {
            LogWarn("No root directory defined");
            continue;
        }
        auto it = loaders.find(root->name);
        if (it != loaders.end())
        {
            root->service_component = it->second.service_component;
            if (root->service_component)
                m_service_components_found = true;

            for (auto file : files)
            {
                if (*it->second.node_name)
                {
                    file->def_node_name = it->second.node_name;
                    file->def_node = base::HashID::make_hash(it->second.node_name);
                    file->def_node_installed = g::pNodesManager->find_node(file->def_node) != nullptr;
                }
                file->service_component = it->second.service_component;
                file->copy_to_cb_only = it->second.copy_to_cb_only;
            }
        }
    }
}

const std::vector<std::unique_ptr<data::ModelItem>>& ModelsManager::get_models() const noexcept
{
    if (!g::pFlow->get_models_ready()) [[unlikely]]
    {
        static std::vector<std::unique_ptr<data::ModelItem>> models;
        return models;
    }
    return m_model_items;
}

void ModelsManager::calc_number_inside(const std::string& filter)
{
    std::function<void(std::vector<std::unique_ptr<data::ModelItem>>&, uint32_t*, bool)> calculator;
    calculator = [this, &calculator, &filter](std::vector<std::unique_ptr<data::ModelItem>>& models, uint32_t* count, bool force_unfilter)
    {
        for (auto& model : models)
        {
            model->models_inside = 0;
            if (!model->children.empty())
            {
                const bool category_unfiltered = force_unfilter || (!filter.empty() && model->search.find(filter) != std::string::npos);
                calculator(model->children, &model->models_inside, category_unfiltered);
                if (count != nullptr)
                    *count += model->models_inside;
            }
            else if (model->is_model)
            {
                if (force_unfilter || filter.empty())
                {
                    model->filtered = false;
                    if (count != nullptr)
                        (*count)++;
                }
                else
                {
                    model->filtered = (model->search.find(filter) == std::string::npos);
                    if (model->filtered && !model->file_name.empty())
                        model->filtered = (model->file_name.find(filter) == std::string::npos);
                    if (count != nullptr)
                        *count += model->filtered ? 0 : 1;
                }
            }
        }
    };

    uint32_t count = 0;
    calculator(m_model_items, &count, false);
}

} // namespace mngr
