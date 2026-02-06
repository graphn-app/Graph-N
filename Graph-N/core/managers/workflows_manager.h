
#pragma once
#include "core/managers/manager.h"
#include "core/data/node.h"

namespace data
{
class Workflow;
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

    WfInfo create(bool skip_name);
    inline void cleanup(wid id) const { find_data(id).json.clear(); }

    inline wid get_active() const { return m_active; }
    void set_active(wid id);

    void set_read_only(wid id);

    inline WfInfo get_info(wid id) const { return from_data(find_data(id)); }
    inline WfData get_data(wid id) const { return find_data(id); }

    void update_name(wid id, const std::string& name);
    void update_history(wid id, uint32_t history_pos, uint32_t history_steps);

    void update_json(wid id, std::string&& json);
    const std::string& get_json(wid id);

    std::vector<WfInfo> get_wfs() const;
    std::vector<WfInfo> get_deleted() const;

    void mark_deleted(wid id);
    void undelete(wid id);

    const std::vector<std::string>& get_labels() const { return m_labels; }
    std::string add_label(const char* label);
    void delete_label(const char* label);

    void add_to_label(wid id, const char* label);
    void remove_from_label(wid id, const char* label);

    bool is_in_label(wid id, const char* label) const;

    data::Workflow* access(wid id) const;

private:
    data::Workflow* create(WfData&& data);

    data::Workflow* find(wid id) const;
    WfData& find_data(wid id) const;

    WfInfo from_data(const WfData& data) const;

    std::vector<WfInfo> get_infos(bool deleted) const;

private:
    std::map<wid, std::unique_ptr<data::Workflow>> m_wfs;
    std::list<std::unique_ptr<data::Workflow>> m_wfs_ro;
    std::vector<std::string> m_labels;
    uint32_t m_wf_name_counter = 0;
    wid m_active = 0;
};

} // namespace mngr
