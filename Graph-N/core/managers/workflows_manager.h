
#pragma once
#include "core/managers/manager.h"
#include "core/data/node.h"

namespace data
{
class Workflow;
class NodeInstance;
}

namespace mngr
{

class WfsManager final : public Manager
{
public:
    WfsManager();
    ~WfsManager() override;

    bool init() override;
    void free() override;
    void stop_all_threads() override;

    WfInfo create(bool skip_name, bool skip_db);
    WfInfo create_copy(wid source);

    data::Workflow* access(wid id) const;

    inline void cleanup(wid id) const { find_data(id).json.clear(); }

    inline wid get_active() const { return m_active; }
    void set_active(wid id);

    void set_read_only(wid id);

    inline WfInfo get_info(wid id) const { return get_info(find(id)); }
    inline WfData& get_data(wid id) const { return find_data(id); }

    void update_name(wid id, const std::string& name);
    void update_history(wid id, uint32_t history_pos, uint32_t history_steps);

    void update_json(wid id, std::string&& json);
    const std::string& get_json(wid id);

    std::vector<WfInfo> get_wfs() const;
    std::vector<WfInfo> get_deleted() const;

    void delete_wf(wid id);
    void undelete_wf(wid id);
    bool is_deleted(wid id) const;

    inline const std::list<std::pair<std::string, uint32_t>>& get_labels() const { return m_labels; }
    const std::list<wid>& get_wfs_in_label(uint32_t label_id) const;
    bool get_is_wf_in_label(wid wf_id, uint32_t label_id) const;

    uint32_t create_label();
    bool rename_label(uint32_t label_id, const std::string& new_name);
    void remove_label(uint32_t label_id);

    void set_wf_labels(wid wf_id, std::set<uint32_t> labels);

    void copy_begin(wid source, std::optional<ImVec2> src_point);
    void copy_add(data::NodeInstance* node);
    void copy_del(data::NodeInstance* node);
    void copy_end(wid dest, std::optional<ImVec2> dest_point);
    inline bool copy_has_data() const { return !m_copied_nodes.empty(); }

private:
    data::Workflow* create(WfData&& data, bool dump_to_db);
    data::Workflow* find(wid id) const;
    WfData& find_data(wid id) const;
    WfInfo get_info(const data::Workflow* wf) const;
    std::vector<WfInfo> get_infos(bool deleted) const;
    void init_labels();
    void save_labels() const;
    void sort_wfs(std::list<wid>& wfs);

private:
    std::map<wid, std::unique_ptr<data::Workflow>> m_wfs;
    std::list<std::unique_ptr<data::Workflow>> m_wfs_ro;
    uint32_t m_wf_name_counter = 0;
    wid m_active = 0;

    std::list<std::pair<std::string, uint32_t>> m_labels;
    uint32_t m_last_label_id = 0;
    std::map<uint32_t, std::list<wid>> m_wfs_in_label;

    wid m_copy_src = 0;
    std::optional<ImVec2> m_copy_point;
    std::set<data::NodeInstance*> m_copied_nodes;
};

} // namespace mngr
