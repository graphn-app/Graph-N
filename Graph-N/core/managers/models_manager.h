
#pragma once
#include "core/managers/manager.h"

namespace data
{
struct ModelItem final
{
    NO_COPY_MOVE_CTORS(ModelItem);
    
    bool is_model = false;
    bool starred = false;

    hid id = 0;
    std::string name;
    std::string file_name;
    std::string search;

    std::string descr;
    std::filesystem::path image_path;

    std::list<hid> nodes;

    std::vector<std::unique_ptr<data::ModelItem>> children;

    uint32_t models_inside = 0;
    uint32_t models_inside_unfiltered = 0;
    bool filtered = false;
};

}

namespace mngr
{

class ModelsManager final : public Manager
{
public:
    ModelsManager() = default;
    ~ModelsManager() override;

    bool init() override;
    void free() override;

    void stop_all_threads() override;

    void load_models();
    void set_nodes(std::unordered_map<std::string, std::list<hid>>&& nodes_map);

    const std::vector<std::unique_ptr<data::ModelItem>>& get_models() const;

    void calc_number_inside(const std::string& filter);

private:
    void load_models_th();
    std::unique_ptr<data::ModelItem> add_file(std::string root_folder, std::filesystem::path file_path);
    void parse_file_info(data::ModelItem* item, std::string info);
    void rebuild_models(const std::vector<std::unique_ptr<data::ModelItem>>& models);
    void sync_to_nodes();
    void sort_models();

private:
    std::thread m_th;
    std::atomic_bool m_models_ready = false;
    std::atomic_bool m_nodes_ready = false;
    std::atomic_bool m_stop_flag = false;

    std::vector<std::unique_ptr<data::ModelItem>> m_model_items;
    std::unordered_map<std::string, std::list<hid>> m_model_to_nodes_map;
};

} // namespace mngr
