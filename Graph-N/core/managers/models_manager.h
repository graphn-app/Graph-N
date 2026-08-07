
#pragma once
#include "core/managers/manager.h"

namespace data
{
struct ModelInfo final
{
    std::string info;
    std::string descr;
    std::string tags;
    std::string trigger_phrase;
    std::filesystem::path image_path;
    int32_t textures_id = 0;

    bool empty() noexcept
    {
        return info.empty() && descr.empty() && tags.empty() && trigger_phrase.empty() && image_path.empty();
    }
};

struct ModelItem final
{
    NO_COPY_MOVE_CTORS(ModelItem);
    
    bool is_model = false;
    bool starred = false;

    hid hash = 0;
    std::string name;
    std::string search;
    std::string file_name;

    ModelInfo info;

    std::list<hid> nodes;

    std::vector<std::unique_ptr<data::ModelItem>> children;

    uint32_t models_inside = 0;
    bool filtered = false;

    hid def_node = 0;
    std::string def_node_name;
    bool def_node_installed = false;
    bool service_component = false;
    bool copy_to_cb_only = false;
};

}

namespace mngr
{

class ModelsManager final : public Manager
{
public:
    ModelsManager() noexcept = default;
    ~ModelsManager() noexcept override;

    void free() override;

    void stop_all_threads() override;

    void load_models();
    void set_nodes_map(std::unordered_map<std::string, std::list<hid>>&& nodes_map) noexcept { m_model_to_nodes_map = std::move(nodes_map); }

    const std::vector<std::unique_ptr<data::ModelItem>>& get_models() const noexcept;
    bool has_service_components() const noexcept { return m_service_components_found; }

    void calc_number_inside(const std::string& filter);

private:
    void load_models_th();
    std::unique_ptr<data::ModelItem> add_file(std::string root_folder, std::filesystem::path file_path);
    void parse_file_info(data::ModelItem* item, std::string&& info);
    void rebuild_models(std::vector<std::unique_ptr<data::ModelItem>>& models);
    void sync_to_nodes();
    void sort_models();
    void setup_model_behaviour();

private:
    std::thread m_th;
    std::stop_source m_stop_flag;

    bool m_service_components_found = false;
    std::vector<std::unique_ptr<data::ModelItem>> m_model_items;
    std::unordered_map<std::string, std::list<hid>> m_model_to_nodes_map;
};

} // namespace mngr
