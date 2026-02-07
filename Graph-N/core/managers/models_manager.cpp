
#include "core/managers/models_manager.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

ModelsManager::~ModelsManager()
{
    LogF;
}

bool ModelsManager::init()
{
    return true;
}

void ModelsManager::free()
{}

void ModelsManager::stop_all_threads()
{
    m_stop_flag = true;
    if (m_th.joinable())
        m_th.join();
}

void ModelsManager::load_models()
{
    m_nodes_ready = false;
    m_models_ready = false;
    m_stop_flag = false;
    m_th = std::thread(&ModelsManager::load_models_th, this);
}

void ModelsManager::set_nodes(std::unordered_map<std::string, std::list<hid>>&& nodes_map)
{
    m_model_to_nodes_map = std::move(nodes_map);
    m_nodes_ready = true;
}

void ModelsManager::load_models_th()
{
    try
    {
        httplib::Client cli(g::pOptsManager->comfyui_host_load(), cast::to_i32(g::pOptsManager->comfyui_port_load()));
        cli.set_keep_alive(true);

        auto res = cli.Get(comfyui_api::models);
        if (m_stop_flag)
        {
            LogS("Models thread aborted");
            return;
        }
        if (!res || res->status != 200 || res->body.empty())
        {
            LogS("Models thread stopped, server reply: " << res->status << ", body: " << res->body.size());
            return;
        }

        std::list<std::string> requests;
        nlohmann::json j = nlohmann::json::parse(res->body);
        for (const auto& jitem : j)
        {
            if (jitem.contains("name"))                
                requests.push_back(jitem["name"]);
        }

        if (m_stop_flag)
        {
            LogS("Models thread aborted");
            return;
        }

        std::vector<std::unique_ptr<data::ModelItem>> models;
        models.reserve(requests.size() * 2);

        for (auto& req : requests)
        {
            std::string req_path = comfyui_api::models;
            req_path += "/" + req;
            res = cli.Get(req_path);
            if (m_stop_flag)
            {
                LogS("Models thread aborted");
                return;
            }
            if (!res || res->status != 200 || res->body.empty())
            {
                LogS("Can't requests model " << req_path << ",  err " << res->status << ", body: " << res->body.size());
                continue;
            }

            j = nlohmann::json::parse(res->body);
            for (const auto& jitem : j)
            {
                if (jitem.contains("name"))
                {
                    std::filesystem::path filename = std::string(jitem["name"]);
                    auto item = add_file(req, filename);
                    if (item->name.empty())
                        continue;

                    req_path = comfyui_api::model_meta;
                    req_path += "/" + req;
                    httplib::Params params;
                    params.emplace("filename", filename.filename().u8string());
                    res = cli.Get(req_path, params, httplib::Headers());
                    if (res && res->status == 200 && !res->body.empty())
                        parse_file_info(item.get(), std::move(res->body));

                    models.push_back(std::move(item));
                    if (m_stop_flag)
                    {
                        LogS("Models thread aborted");
                        return;
                    }
                }
            }
        }

        rebuild_models(models);

        using namespace std::chrono_literals;
        while (!m_nodes_ready && !m_stop_flag)
        {
            std::this_thread::sleep_for(100ms);
        }

        if (m_stop_flag)
        {
            LogS("Models thread aborted");
            return;
        }

        sync_to_nodes();
        sort_models();
        calc_number_inside("");
        
        m_models_ready = true;
    }
    catch (const std::exception& ex)
    {
        LogExcep(ExcepCategory::ComfyUI, "Models thread err " << ex.what());
    }
}

std::unique_ptr<data::ModelItem> ModelsManager::add_file(std::string root_folder, std::filesystem::path file_path)
{
    auto root = std::make_unique<data::ModelItem>();
    root->is_model = false;
    root->name = root_folder;

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
            auto ptr = fn.get();
            parent->children.push_back(std::move(fn));
            parent = ptr;
        }
    }

    auto file = std::make_unique<data::ModelItem>();
    file->is_model = true;
    file->name = file_path.filename().u8string();
    parent->children.push_back(std::move(file));

    return root;
}

void ModelsManager::parse_file_info(data::ModelItem* item, std::string info)
{
    while (!item->children.empty())
    {
        item = item->children.begin()->get();
    }

    const auto j = nlohmann::json::parse(info);
    if (j.contains("modelspec.title"))
    {
        item->descr = "Filename: " + item->name + "\n";
        item->file_name = item->name;
        item->name = j["modelspec.title"];
    }
    if (j.contains("modelspec.author"))
    {
        if (!item->descr.empty())
            item->descr += "\n";
        item->descr += "Author: " + std::string(j["modelspec.author"]);
    }
    if (j.contains("modelspec.architecture"))
    {
        if (!item->descr.empty())
            item->descr += "\n";
        item->descr += "Architecture: " + std::string(j["modelspec.architecture"]);
    }
    if (j.contains("modelspec.date"))
    {
        if (!item->descr.empty())
            item->descr += "\n";
        item->descr += "Date: " + std::string(j["modelspec.date"]);
    }
    if (j.contains("modelspec.description"))
    {
        if (!item->descr.empty())
            item->descr += "\n";
        item->descr += std::string(j["modelspec.description"]);
    }
    if (j.contains("modelspec.thumbnail"))
    {
        auto temp = fs::get_temp_directory_path();
        if (tools::path::make_folder(temp))
        {
            auto filename = tools::str::generate_uuid_v4() + ".image";
            std::string image = j["modelspec.thumbnail"];
            auto pos = image.find(",");
            if (pos != std::string::npos)
            {
                const char* base64 = image.c_str() + ++pos;
                const size_t len = image.size() - pos;
                const auto binary = tools::str::base64_decode(base64, len);
                temp /= filename;
                if (tools::path::make_file(temp, true, binary))
                {
                    item->image_path = temp;
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
                new_item->name = std::move(model->name);
                new_item->file_name = std::move(model->file_name);
                new_item->descr = std::move(model->descr);
                new_item->image_path = std::move(model->image_path);

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

const std::vector<std::unique_ptr<data::ModelItem>>& ModelsManager::get_models() const
{
    if (!m_models_ready)
    {
        static std::vector<std::unique_ptr<data::ModelItem>> models;
        return models;
    }
    return m_model_items;
}

void ModelsManager::calc_number_inside(const std::string& filter)
{
    std::function<void(std::vector<std::unique_ptr<data::ModelItem>>&, uint32_t*, bool)> calculator;
    calculator = [&calculator, &filter](std::vector<std::unique_ptr<data::ModelItem>>& models, uint32_t* count, bool force_unfilter)
    {
        for (auto& model : models)
        {
            model->models_inside = model->models_inside_unfiltered = 0;
            if (!model->children.empty())
            {
                const bool category_unfiltered = force_unfilter || (!filter.empty() && model->search.find(filter) != std::string::npos);
                model->models_inside_unfiltered = 0;
                calculator(model->children, filter.empty() ? &model->models_inside : &model->models_inside_unfiltered, category_unfiltered);
                if (count != nullptr)
                    *count += filter.empty() ? model->models_inside : model->models_inside_unfiltered;
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
