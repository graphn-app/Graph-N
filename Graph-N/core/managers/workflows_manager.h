
#pragma once
#include "core/managers/manager.h"
#include "core/workflow/workflow.h"
#include "core/data/node.h"
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
    WfsManager();
    ~WfsManager() override;

    bool init() override;
    void free() override;
    void stop_all_threads() override;

    const WfData& create(bool generate_name, bool dump_to_db, bool ro);
    const WfData& create_copy(wid source);
    bool load(wid wf_id, blob::Blob& drawer_state);

    data::Workflow* access(wid id) const;

    inline wid get_active() const { return m_active; }
    void set_active(wid id);

    inline const WfData& get_data(wid id) const { return access(id)->get_data(); }
    inline WfData& get_data(wid id) { return access(id)->get_data(); }

    void update_name(wid id, const std::string& name);

    std::vector<WfData*> get_wfs() const;
    std::vector<WfData*> get_undeleted() const;
    std::vector<WfData*> get_deleted() const;

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

    void close_and_purge();
    void close(wid wf_id);
    void save(wid wf_id);
    void revert(wid wf_id, blob::Blob& drawer_state);
    void dump(wid wf_id) const;

    wid import_wf(std::filesystem::path& from);
    void export_wf(bool api, std::filesystem::path& to);

private:
    data::Workflow* create(WfData&& data, bool dump_to_db);
    void init_labels();
    void save_labels() const;
    void sort_wfs(std::list<wid>& wfs);

    // Timer
    void on_1s_lazy_timer(int64_t total_seconds_elapsed) override;
    void on_frame(int64_t /*total_frames_elapsed*/) override {}

private:
    std::unordered_map<wid, std::unique_ptr<data::Workflow>> m_wfs;
    std::list<std::unique_ptr<data::Workflow>> m_wfs_ro;
    uint32_t m_wf_name_counter = 0;
    wid m_active = 0;

    std::list<std::pair<std::string, uint32_t>> m_labels;
    uint32_t m_last_label_id = 0;
    std::unordered_map<uint32_t, std::list<wid>> m_wfs_in_label;

    wid m_copy_src = 0;
    std::optional<ImVec2> m_copy_point;
    std::set<data::NodeInstance*> m_copied_nodes;

    int64_t m_dump_counter = 0;
};

} // namespace mngr
