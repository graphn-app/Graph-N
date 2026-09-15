#pragma once
#include "core/managers/manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node.h"
#include "core/listeners.h"

namespace data
{
class NodeInstance;
}

namespace mngr
{

class WfsManager final
    : public Manager
    , public listener::Timer
{
public:
    WfsManager() noexcept;
    ~WfsManager() noexcept override;

    bool init() override;
    void free() override;
    void stop_all_threads() noexcept override;

    const WfData& create(bool generate_name, bool dump_to_db, bool ro);
    const WfData& create_copy(wid source);
    std::shared_ptr<data::Workflow> create_copy_virtual(const data::Workflow* source);
    const WfData& create_temp(const blob::Blob& source, const std::string& name, const std::string& label);

    bool is_loaded(wid wf_id) const;
    bool load(wid wf_id, blob::Blob& drawer_state);

    data::Workflow* access(wid id, bool allow_zero_ret = false) const;

    wid get_active() const noexcept { return m_active; }
    void set_active(wid id);

    const WfData& get_data(wid id) const { return access(id)->get_data(); }
    WfData& get_data(wid id) { return access(id)->get_data(); }

    void update_name(wid id, const std::string& name);

    size_t get_wfs_count() const noexcept { return m_wfs.size(); }
    std::vector<WfData*> get_wfs() const;
    std::vector<WfData*> get_undeleted() const;
    std::vector<WfData*> get_deleted() const;

    void delete_wf(wid id);
    void undelete_wf(wid id);
    bool is_deleted(wid id) const;
    void erase_wf(wid id);

    void remove_virtual_wf(wid id);

    const std::list<Label>& get_labels() const noexcept { return m_labels.get_label_list(); }
    std::list<wid> get_wfs_in_label(uint32_t label_id) const;
    bool get_is_wf_in_label(wid wf_id, uint32_t label_id) const;

    uint32_t create_label(bool auto_del = false);
    uint32_t find_label(const std::string& label_name) const;
    bool rename_label(uint32_t label_id, const std::string& new_name);
    void remove_label(uint32_t label_id);

    void set_wf_labels(wid wf_id, std::set<uint32_t> labels);

    void copy_begin(const data::Workflow* wf_src, std::optional<ImVec2> src_point);
    void copy_add(data::NodeInstance* node);
    void copy_del(data::NodeInstance* node);
    void copy_end(data::Workflow* wf_dest, std::optional<ImVec2> dest_point);
    bool copy_has_data() const noexcept { return !m_copied_nodes.empty(); }

    void close_and_purge();
    void close(wid wf_id);
    void save(wid wf_id);
    void revert(wid wf_id, blob::Blob& drawer_state);
    void dump(wid wf_id) const;

    wid import_wf(const std::filesystem::path& from);
    wid import_wf_from_json(const std::string& jdata, const std::string& name, tools::dt use_time = {}, bool virt = false);
    void export_wf(wid wf_id, bool api, const std::filesystem::path& to);

private:
    data::Workflow* create(WfData&& data, bool dump_to_db);
    bool is_auto_label(uint32_t label_id) const;
    void sort_wfs(std::list<wid>& wfs);

    // Timer
    void on_1s_lazy_timer(int64_t total_seconds_elapsed) override;

private:
    std::unordered_map<wid, std::unique_ptr<data::Workflow>> m_wfs;
    std::list<std::unique_ptr<data::Workflow>> m_wfs_ro;
    std::list<std::shared_ptr<data::Workflow>> m_wfs_virt;
    uint32_t m_wf_name_counter = 0;
    wid m_active = 0;

    LabelsList m_labels;

    const data::Workflow* m_copy_src = nullptr;
    std::optional<ImVec2> m_copy_point;
    std::set<data::NodeInstance*> m_copied_nodes;

    int64_t m_dump_counter = 0;
};

} // namespace mngr
