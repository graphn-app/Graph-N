
#pragma once
#include "core/base/structs.h"
#include "core/managers/manager.h"
#include "core/listeners.h"

namespace data
{
class Workflow;
}

namespace mngr
{

struct ExecGroup final
{
    uint64_t group_id = 0;
    int32_t tasks_num = 0;                  // num of tasks in group, -1 if infinite, 0 - stop processing this group
    std::shared_ptr<data::Workflow> wf;     // current wf
    wid src_wf = 0;                         // original wf, might be 0
    std::string wf_name;                    // original wf' name
    ExecTask task;                          // current task
    int32_t nodes_count = 0;                // nodes in wf

    enum class State { None, Created, Prompted, Executing, Finished };
    State state = State::None;

    std::list<ExecTask> completed;          // list of completed tasks

    ExecGroup copy() const
    {
        ExecGroup g;
        g.group_id = group_id;
        g.src_wf = src_wf;
        g.wf_name = wf_name;
        return g;
    }
};

class QueueManager final
    : public Manager
    , public IsAlive
    , public listener::ServerRestartHandler
{
public:
    QueueManager() noexcept;
    ~QueueManager() noexcept override;

    bool init() override;
    void stop_all_threads() noexcept override;

    void process();

    void add_to_queue(data::Workflow* wf, int32_t count, bool push_front);

    const ExecTask* get_is_failed(wid wf_id) const;
    bool get_is_queued(wid wf_id) const { return m_queued_wfs.contains(wf_id); }

    void interrupt_task(wid wf_id = 0);

    bool is_paused() const noexcept { return m_paused; }
    void toggle_pause() noexcept { m_paused = !m_paused; }

    int32_t is_external_task() const noexcept { return m_external_tasks; }

    bool is_dead_task() const noexcept { return m_task_elapsed_time > 20; /*QUEUE_DEAD_TASK;*/ };

    void modify_group(uint64_t group_id, int32_t num_of_tasks);
    void drop_tasks(uint64_t group_id);
    void drop_wf_tasks(wid wf_id);
    void drop_all() noexcept;

    const ExecGroup* get_current_group() const noexcept { return m_current; }
    std::vector<ExecGroup>& get_past_tasks() noexcept { return m_queue_past; }
    const std::list<ExecGroup>& get_next_tasks() const noexcept { return m_queue_next; }
    void create_from_past_task(uint64_t group, uint64_t task);

    void swap_groups(uint64_t group_l, uint64_t group_r);

private:
    void parse_http_response(int32_t result, const std::string& payload);
    void check_ws_inbox();
    void go_next_task();
    void load_outputs(ExecTask& task);
    void load_output_data(uint64_t task_id, uint64_t out_id, const std::string&& binary);
    void check_current_task_is_done();

    // ServerRestartHandler
    void handle_server_killed() override;

private:
    int32_t m_external_tasks = 0;
    double m_task_active_time = 0.0;
    double m_task_elapsed_time = 0.0;

    bool m_can_check_ws = true;
    bool m_paused = false;
    bool m_lost = false;
    ExecGroup* m_current = nullptr;
    ExecGroup m_current_group;
    uint32_t m_run_no = 0;
    std::list<ExecGroup> m_queue_next;
    std::vector<ExecGroup> m_queue_past;
    std::map<wid, ExecTask*> m_failed_wfs;
    std::set<wid> m_queued_wfs;
};

} // namespace mngr
