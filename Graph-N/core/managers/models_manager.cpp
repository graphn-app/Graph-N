
#include "core/managers/models_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

ModelsManager::~ModelsManager()
{
    LogF;
}

bool ModelsManager::init()
{
    LogF;
    if (!Manager::init())
        return false;
    return true;
}

void ModelsManager::free()
{
    m_model_items.clear();
    m_model_to_nodes_map.clear();
    m_service_components_found = false;
}

void ModelsManager::stop_all_threads()
{
    m_stop_flag = true;

    if (m_th.joinable())
        m_th.join();
}

void ModelsManager::load_models()
{
    stop_all_threads();
    free();

    m_stop_flag.store(false, std::memory_order_relaxed);
    m_th = std::thread(&ModelsManager::load_models_th, this);
}

void ModelsManager::load_models_th()
{
    try
    {
        g::pOptsManager->model_starred_preload();

        httplib::Client cli(g::pOptsManager->comfyui_host_load(), cast::to_i32(g::pOptsManager->comfyui_port_load()));
        cli.set_keep_alive(true);

        auto res = cli.Get(comfyui_http::models);
        if (m_stop_flag.load(std::memory_order_relaxed))
        {
            Log("Models thread aborted");
            return;
        }
        if (!res || res->status != 200 || res->body.empty())
        {
            Log("Models thread stopped, server reply: " << res->status << ", body: " << res->body.size());
            return;
        }

        std::list<std::string> requests;
        auto j = u_json::parse(res->body);
        for (const auto& jitem : j)
        {
            if (jitem.contains("name"))                
                requests.push_back(jitem["name"]);
        }

        if (m_stop_flag.load(std::memory_order_relaxed))
        {
            Log("Models thread aborted");
            return;
        }

        std::vector<std::unique_ptr<data::ModelItem>> models;
        models.reserve(requests.size() * 2);

        for (auto& req : requests)
        {
            std::string req_path = comfyui_http::models;
            req_path += "/" + req;
            res = cli.Get(req_path);
            if (!res || res->status != 200 || res->body.empty())
            {
                Log("Can't requests model " << req_path << ",  err " << res->status << ", body: " << res->body.size());
                continue;
            }

            j = u_json::parse(res->body);
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

                        req_path = comfyui_http::model_meta;
                        req_path += "/" + req;
                        httplib::Params params;
                        params.emplace("filename", filename.filename().u8string());
                        res = cli.Get(req_path, params, httplib::Headers());
                        if (res && res->status == 200 && !res->body.empty())
                            parse_file_info(item.get(), std::move(res->body));

                        models.push_back(std::move(item));
                    }
                }
            }

            if (m_stop_flag.load(std::memory_order_relaxed))
            {
                Log("Models thread aborted");
                return;
            }
        }

        rebuild_models(models);

        while (!g::pFlow->get_nodes_ready() && !m_stop_flag.load(std::memory_order_relaxed))
        {
            std::this_thread::sleep_for(100ms);
        }

        if (m_stop_flag.load(std::memory_order_relaxed))
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
        LogExcep(ExcepCategory::ComfyUI, "Models thread err " << ex.what());
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
            fn->name = (*part).u8string();
            fn->search = tools::str::to_lower_ascii(fn->name);
            auto ptr = fn.get();
            parent->children.push_back(std::move(fn));
            parent = ptr;
        }
    }

    auto file = std::make_unique<data::ModelItem>();
    file->is_model = true;
    file->name = file_path.filename().u8string();
    file->search = tools::str::to_lower_ascii(file->name);
    file->id = base::HashID::make_hash(file->name);
    const auto starred = g::pOptsManager->model_starred_load(file->id);
    if (starred.has_value())
        file->starred = starred.value();
    parent->children.push_back(std::move(file));

    return root;
}

void ModelsManager::parse_file_info(data::ModelItem* item, std::string info)
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
                const size_t len = image.size() - pos;
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

void ModelsManager::rebuild_models(const std::vector<std::unique_ptr<data::ModelItem>>& models)
{
    std::function<data::ModelItem*(const std::vector<std::unique_ptr<data::ModelItem>>&, const std::string&, int32_t, int32_t)> find_item;
    find_item = [&find_item]
        (const std::vector<std::unique_ptr<data::ModelItem>>& models, const std::string& name, int32_t search_depth, int32_t curr_depth) -> data::ModelItem*
    {
        if (search_depth < curr_depth)
            return nullptr;

        for (auto& model : models)
        {
            if (search_depth == curr_depth && model->name == name)
                return model.get();
            auto item = find_item(model->children, name, search_depth, curr_depth + 1);
            if (item != nullptr)
                return item;
        }

        return nullptr;
    };

    m_model_items.clear();
    m_model_items.reserve(models.size());

    std::function<void(const std::vector<std::unique_ptr<data::ModelItem>>&, std::vector<std::unique_ptr<data::ModelItem>>&, int32_t)> add_items;
    add_items = [this, &add_items, &find_item]
        (const std::vector<std::unique_ptr<data::ModelItem>>& models, std::vector<std::unique_ptr<data::ModelItem>>& dest, int32_t depth)
    {
        for (auto it = models.begin(); it != models.end(); ++it)
        {
            auto& model = *it;
            auto item = find_item(m_model_items, model->name, depth, 0);
            if (nullptr == item)
            {
                auto new_item = std::make_unique<data::ModelItem>();
                new_item->is_model = model->is_model;
                new_item->starred = model->starred;
                new_item->id = model->id;
                new_item->name = std::move(model->name);
                new_item->search = std::move(model->search);
                new_item->file_name = std::move(model->file_name);
                new_item->info = std::move(model->info);

                item = new_item.get();
                dest.push_back(std::move(new_item));
            }

            add_items(model->children, item->children, depth + 1);
        }
    };

    add_items(models, m_model_items, 0);
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
    const auto sort_callback = [](const std::unique_ptr<data::ModelItem>& lhs, const std::unique_ptr<data::ModelItem>& rhs)
    {
        if (!lhs->is_model && rhs->is_model)
            return true;
        if (lhs->is_model && !rhs->is_model)
            return false;

        return tools::str::to_lower_ascii(lhs->name) < tools::str::to_lower_ascii(rhs->name);
    };

    std::function<void(std::vector<std::unique_ptr<data::ModelItem>>&)> go_sort;
    go_sort = [&go_sort, &sort_callback](std::vector<std::unique_ptr<data::ModelItem>>& models)
    {
        std::sort(models.begin(), models.end(), sort_callback);
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

const std::vector<std::unique_ptr<data::ModelItem>>& ModelsManager::get_models() const
{
    if (!g::pFlow->get_models_ready())
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
    Log("Filter = (" << filter << "), count=" << count);
}

} // namespace mngr
