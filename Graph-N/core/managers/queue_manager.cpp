#include "core/managers/queue_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/connect_manager.h"
#include "core/managers/db_manager.h"
#include "core/workflow/serializer/wf_serializers.h"
#include "core/base/comfyui_web_paths.h"
#include "ui/custom_widgets/custom_widget.h"
#include "ui/graph-n_flow.h"
#include "ui/helpers/textures.h"

namespace mngr
{

QueueManager::QueueManager() noexcept = default;
QueueManager::~QueueManager() noexcept = default;

void QueueManager::stop_and_cleanup() noexcept
{
    interrupt_task();
    dead();
}

bool QueueManager::init()
{
    if (!Manager::init())
        return false;

    m_queue_past.reserve(500);
    return true;
}

namespace
{
std::string get_log_task_id(const ExecGroup& group)
{
    if (!group.task.prompt_id.empty())
        return fmt::format(FMT_STRING("{}/{}/{}"), group.group_id, group.task.task_id, group.task.prompt_id);
    if (group.task.task_id != 0)
        return fmt::format(FMT_STRING("{}/{}"), group.group_id, group.task.task_id);
    return fmt::format(FMT_STRING("{}"), group.group_id);
}

std::string get_log_task_id(const ExecTask& task)
{
    if (!task.prompt_id.empty())
        return fmt::format(FMT_STRING("{}/{}"), task.task_id, task.prompt_id);
    return fmt::format(FMT_STRING("{}"), task.task_id);
}

const char* get_result_name(TaskResult result) noexcept
{
    switch (result)
    {
    case TaskResult::Failed:
        return "Failed";
    case TaskResult::InProgress:
        return "InProgress";
    case TaskResult::Interrupted:
        return "Interrupted";
    case TaskResult::Lost:
        return "Lost";
    case TaskResult::Success:
        return "Success";
    case TaskResult::None:
        return "None";
    }
    LogRErrorS("Unknown task result", "Unknown");
}
} // namespace

void QueueManager::handle_server_killed()
{
    m_external_tasks = 0;
    if (nullptr == m_current)
        return;

    Log("Server offline, stop task #" << get_log_task_id(*m_current));
    m_current->state = ExecGroup::State::Finished;
    m_current->task.result = TaskResult::Failed;
    go_next_task();
}

void QueueManager::interrupt_task(wid wf_id /*= 0*/)
{
    if (nullptr == m_current)
        return;

    if (!g::pFlow->get_online())
    {
        handle_server_killed();
        return;
    }

    if (wf_id != 0 && m_current->src_wf != wf_id)
        return;

    const auto task_log_id = get_log_task_id(*m_current);
    const auto callback = [this, task_log_id](int32_t result, std::string&& payload)
    {
        LogS("Task #" << task_log_id << " http res " << result << ", payload sz " << payload.size());
    };

    Log("Interrupt task #" << task_log_id);
    g::pFlow->http_post(HttpRequestor::Queue_Interrupt, get_alive_callback(), comfyui_http::interrupt, "", callback, true /*retry on timeout*/);
}

void QueueManager::drop_all() noexcept
{
    m_queue_next.clear();
    m_queued_wfs.clear();

    if (m_current != nullptr)
        m_current->tasks_num = 0;

    Log("All tasks are dropped");
}

void QueueManager::add_to_queue(data::Workflow* wf, int32_t count, bool push_front)
{
    LogRAssert(wf, "Zero ptr", );
    if (count <= 0)
        count = -1;

    ExecGroup group;
    group.tasks_num = count;
    group.src_wf = wf->get_id();
    group.group_id = base::GlobalID::random();
    group.wf_name = wf->get_data().name;
    group.wf = g::pWfsManager->create_copy_virtual(wf);
    group.nodes_count = cast::to_i32(wf->get_nodes_all().size());
    group.state = ExecGroup::State::None;

    Log("Group #" << get_log_task_id(group) << " added to the " << (push_front ? "front" : "end") << ": " << count << ". Src: '" << wf->get_data().name << "', #" << wf->get_id());

    m_queued_wfs.insert(wf->get_id());
    if (push_front)
        m_queue_next.push_front(std::move(group));
    else
        m_queue_next.push_back(std::move(group));
}

void QueueManager::process()
{
    if (!g::pFlow->get_online()) [[unlikely]]
    {
        m_external_tasks = 0;
        if (m_current != nullptr)
            m_lost = true;
        return;
    }

    check_ws_inbox();

    if (nullptr == m_current || ExecGroup::State::None == m_current->state || ExecGroup::State::Finished == m_current->state)
    {
        go_next_task();
        return;
    }

    switch (m_current->state)
    {
    case ExecGroup::State::Executing:
        if (m_lost)
            check_current_task_is_done();
        break;

    case ExecGroup::State::Prompted:
        if (m_lost)
            check_current_task_is_done();
        break;

    case ExecGroup::State::Created:
        {
            auto& task = m_current->task;
            task.run_no = ++m_run_no;
            task.time_start = tools::dt::from_current_date().time();
            task.time_start_s = tools::dt::from_time(task.time_start).as_full_date_readable(false);
            task.result = TaskResult::InProgress;

            o_json j;
            j["client_id"] = base::GlobalID::client_id();
            j["extra_data"]["extra_pnginfo"]["workflow"] = std::move(task.json);
            j["prompt"] = std::move(task.api);

            const auto log_task_id = get_log_task_id(*m_current);
            const auto callback = [this, log_task_id](int32_t result, std::string&& payload)
            {
                Log("Task #" << log_task_id << " responded, http res " << result << ", payload sz " << payload.size());
                parse_http_response(result, payload);
                m_can_check_ws = true;
            };

            Log("Task #" << log_task_id << " posted. Wf #" << m_current->src_wf);
            m_can_check_ws = false;
            g::pFlow->http_post(HttpRequestor::Queue_Task, get_alive_callback(), comfyui_http::prompt, j.dump(), callback);
            m_current->state = ExecGroup::State::Prompted;
        }
        break;

    default:
        LogError("Unknown task state");
    }
}

void QueueManager::go_next_task()
{
    const auto make_wf = [](ExecTask& task, data::Workflow* wf, wid src_id)
    {
        task.json = data::WfSerializerToJson(wf).serialize();
        task.api = data::WfSerializerToJsonApi(wf).serialize();
        task.flow = data::WfSerializerToBlob(wf).serialize({}).release();
        task.search_data = wf->collect_search_data();
        wf->increment(0 == src_id ? nullptr : g::pWfsManager->access(src_id, true));
    };

    if (nullptr == m_current)
    {
        if (m_queue_next.empty() || is_paused())
            return;

        auto& group = m_queue_next.front();
        m_current_group = std::move(group);
        m_current = &m_current_group;
        m_queued_wfs.erase(m_current->src_wf);
        m_queue_next.pop_front();

        m_current_group.task.task_id = base::GlobalID::random();
        m_current_group.task.active = true;
        m_current_group.state = ExecGroup::State::Created;
        make_wf(m_current_group.task, m_current_group.wf.get(), m_current_group.src_wf);
        m_failed_wfs[m_current_group.src_wf] = nullptr;
        Log("Started new task #" << get_log_task_id(m_current_group) << " from wf " << m_current_group.src_wf);
        return;
    }

    if (ExecGroup::State::Finished == m_current->state)
    {
        m_task_active_time = 0.0f;
        Log("Task " << get_log_task_id(*m_current) << " is finished for wf " << m_current->src_wf << ", result " << get_result_name(m_current->task.result));

        if (m_queue_past.empty())
            m_queue_past.push_back(m_current_group.copy());

        if (m_queue_past.back().src_wf != m_current_group.src_wf)
            m_queue_past.push_back(m_current_group.copy());

        ExecGroup& g = m_queue_past.back();
        const auto result = m_current->task.result;
        m_current_group.task.active = false;
        m_current_group.task.group_id = g.group_id;
        g.completed.push_front(std::move(m_current_group.task));
        m_current_group.task = {};

        auto& t = g.completed.front();

        if (result != TaskResult::Success && result != TaskResult::Lost) [[unlikely]]
        {
            Log("Dropped all tasks due to error");
            m_current->tasks_num = 0;
            g::pWfsManager->erase_wf(m_current->wf->get_id());
            t.search_data.clear();
            t.search_data.shrink_to_fit();

            m_failed_wfs[m_current->src_wf] = &g.completed.front();

            m_current = nullptr;
            m_current_group = {};
            return;
        }

        g::pDbManager->history_add(t);

        if (0 == m_current->tasks_num || --m_current->tasks_num == 0)
        {
            Log("Finished all tasks for wf " << m_current_group.src_wf);
            g::pWfsManager->erase_wf(m_current->wf->get_id());
            m_current = nullptr;            
            m_current_group = {};
            return;
        }

        if (is_paused()) [[unlikely]]
        {
            m_queue_next.push_front(std::move(m_current_group));
            m_current = nullptr;
            return;
        }

        m_current_group.task.task_id = base::GlobalID::random();
        m_current_group.task.active = true;
        m_current_group.state = ExecGroup::State::Created;
        make_wf(m_current_group.task, m_current_group.wf.get(), m_current_group.src_wf);
        m_failed_wfs[m_current_group.src_wf] = nullptr;
        Log("Created new task for wf " << m_current_group.src_wf << ": #" << get_log_task_id(m_current_group));
        return;
    }

    LogError("Current group in wrong state");
}

const ExecTask* QueueManager::get_is_failed(wid wf_id) const
{
    auto it = m_failed_wfs.find(wf_id);
    if (it != m_failed_wfs.end())
        return it->second;
    return nullptr;
}

void QueueManager::modify_group(uint64_t group_id, int32_t num_of_tasks)
{
    auto it = std::ranges::find_if(m_queue_next, [group_id](const ExecGroup& g) { return g.group_id == group_id; });
    if (it != m_queue_next.end())
    {
        auto& g = *it;
        Log("Group #" << group_id << " modified: " << g.tasks_num << " -> " << num_of_tasks);
        g.tasks_num = num_of_tasks;
    }

    if (m_current_group.group_id == group_id)
    {
        Log("Group #" << group_id << " modified: " << m_current_group.tasks_num << " -> " << num_of_tasks);
        m_current_group.tasks_num = num_of_tasks;
    }
}

void QueueManager::drop_tasks(uint64_t group_id)
{
    for (auto it = m_queue_next.begin(); it != m_queue_next.end(); )
    {
        auto& group = *it;
        if (group.group_id == group_id)
            it = m_queue_next.erase(it);
        else
            ++it;
    }

    if (m_current_group.group_id == group_id)
        m_current_group.tasks_num = 0;

    Log("Group #" << group_id << ": all tasks dropped");
}

void QueueManager::drop_wf_tasks(wid wf_id)
{
    for (auto it = m_queue_next.begin(); it != m_queue_next.end(); )
    {
        auto& group = *it;
        if (group.src_wf == wf_id)
            it = m_queue_next.erase(it);
        else
            ++it;
    }

    if (m_current_group.src_wf == wf_id)
        m_current_group.tasks_num = 0;

    Log("Wf #" << wf_id << ": all tasks dropped" << (m_current_group.src_wf == wf_id ? " (this is current group)" : ""));
}

void QueueManager::swap_groups(uint64_t group_l, uint64_t group_r)
{
    auto it_l = std::ranges::find_if(m_queue_next, [group_l](const ExecGroup& g) noexcept { return g.group_id == group_l; });
    auto it_r = std::ranges::find_if(m_queue_next, [group_r](const ExecGroup& g) noexcept { return g.group_id == group_r; });
    if (it_l != m_queue_next.end() && it_r != m_queue_next.end())
        std::iter_swap(it_l, it_r);
}

void QueueManager::create_from_past_task(uint64_t group, uint64_t task)
{
    const auto it_g = std::ranges::find_if(m_queue_past, [group](const ExecGroup& g) { return g.group_id == group; });
    if (it_g == m_queue_past.end())
        return;

    const auto it_t = std::ranges::find_if(it_g->completed, [task](const ExecTask& t) { return t.task_id == task; });
    if (it_t == it_g->completed.end())
        return;

    const auto& blob = it_t->flow;
    auto& data = g::pWfsManager->create_temp(blob, "Q:" + it_g->wf_name + ":" + std::to_string(it_t->run_no), TEMP_LABEL_COMPLETED);
    it_t->open_id = data.id;
    Log("New wf '" << "Q:" + it_g->wf_name + ":" + std::to_string(it_t->run_no) << "' created from task");
    g::pFlow->wf_activate(data.id);
}

namespace
{
void append_err(std::string& full, const std::string& err)
{
    if (!err.empty())
    {
        if (full.empty()) full = err;
        else full += "\r\n" + err;
    }
}
} // namespace

void QueueManager::parse_http_response(int32_t result, const std::string& payload)
{
    if (nullptr == m_current)
        return;

    auto& task = m_current->task;
    auto task_log_id = get_log_task_id(*m_current);

    if (result != HTTP_RESP_OK) [[unlikely]]
    {
        task.result = TaskResult::Failed;        
        m_current->state = ExecGroup::State::Finished;

        if (HTTP_RESP_TIMEOUT == result)
        {
            LogS("Task #" << task_log_id << " failed: HTTP request timeout");
            task.err_message = task.err_full = "HTTP request timeout";
            return;
        }

        if (result != HTTP_RESP_BAD_REQUEST)
        {
            LogS("Task #" << task_log_id << " failed: Http post failed: " << result);
            task.err_message = task.err_full = "HTTP post failed, err " + std::to_string(result);
            return;
        }
    }

    try
    {
        const auto j = u_json::parse(payload);
        if (j.contains("prompt_id") && j["prompt_id"].is_string())
        {
            task.prompt_id = j["prompt_id"];
            task_log_id = get_log_task_id(*m_current);
            LogS("Task #" << task_log_id << " prompt_id assigned");
        }
        else [[unlikely]]
        {
            task.result = TaskResult::Failed;
            task.err_message = task.err_full = "Unknown http format";
            LogS("Task #" << task_log_id << " failed: Unknown http format");
        }

        if (j.contains("error") && j["error"].contains("message") && j["error"]["message"].is_string())
        {
            task.err_message = task.err_full = j["error"]["message"];
            LogS("Task #" << task_log_id << " failed: " << task.err_message);
        }

        if (j.contains("node_errors") && j["node_errors"].is_object())
        {
            task.err_nodes.clear();
            auto& jerrors = j["node_errors"];
            for (auto&[jnode_id, jerr] : jerrors.items())
            {
                const iid node = std::strtoul(jnode_id.c_str(), nullptr, 10);
                if (node > 0 && jerr.contains("errors") && jerr["errors"].is_array())
                {
                    for (auto& jitem : jerr["errors"])
                    {
                        if (jitem.is_object())
                        {
                            if (jitem.contains("message") && jitem["message"].is_string())
                                append_err(task.err_full, jitem["message"].get<std::string>());
                            if (jitem.contains("details") && jitem["details"].is_string())
                                append_err(task.err_full, jitem["details"].get<std::string>());
                            if (jitem.contains("extra_info") &&
                                jitem["extra_info"].contains("input_name") &&
                                jitem["extra_info"]["input_name"].is_string())
                            {
                                task.err_nodes[node].insert(jitem["extra_info"]["input_name"].get<std::string>());
                            }
                        }
                    }
                }
            }
        }
    }
    catch(const std::exception& ex)
    { 
        task.result = TaskResult::Failed;
        task.err_message = task.err_full = "HTTP reply parsing failed";
        m_current->state = ExecGroup::State::Finished;

        LogExcep("Response for #" << task_log_id << " parsing err " << ex.what());
        return;
    }    

    if (task.result != TaskResult::InProgress)
        m_current->state = ExecGroup::State::Finished;
    else
        m_current->state = ExecGroup::State::Executing;
}

void QueueManager::check_ws_inbox()
{
    if (!m_can_check_ws)
        return;

    if (m_task_active_time > 0.0f)
        m_task_elapsed_time = ImGui::GetTime() - m_task_active_time;

    const auto callback = [this](bool is_binary, std::string& message)
    {
        if (is_binary) [[unlikely]]
        {
            Log("Binary message of " << message.size() << " bytes. Ignore in MVP"); // TODO: handle binary data
            return;
        }

        try
        {

            auto j = u_json::parse(message);
            if (!j.contains("type") || !j["type"].is_string() || !j.contains("data") || !j["data"].is_object())
                return;

            std::string prompt_id;
            if (j["data"].contains("prompt_id") && j["data"]["prompt_id"].is_string())
            {
                prompt_id = j["data"]["prompt_id"].get<std::string>();
            }
            else if (j["data"].contains("status") &&
                j["data"]["status"].contains("exec_info") &&
                j["data"]["status"]["exec_info"].contains("queue_remaining") &&
                j["data"]["status"]["exec_info"]["queue_remaining"].is_number_integer())
            {
                m_external_tasks = j["data"]["status"]["exec_info"]["queue_remaining"].get<int32_t>();
                m_task_active_time = (0 == m_external_tasks ? 0.0 : ImGui::GetTime());
                return;
            }

            if (nullptr == m_current || m_current->state != ExecGroup::State::Executing || m_current->task.prompt_id != prompt_id)
                return;
            auto& task = m_current->task;
            const auto task_log_id = get_log_task_id(*m_current);

            m_external_tasks = 0;
            m_task_active_time = ImGui::GetTime();

            m_lost = false;
            const std::string& type = j["type"].get_ref<std::string&>();
            if (type == "execution_start")
            {
                if (j["data"].contains("timestamp") && j["data"]["timestamp"].is_number_integer())
                    task.time_start = j["data"]["timestamp"].get<uint64_t>();
                task.time_start_s = tools::dt::from_time(task.time_start).as_full_date_readable(false);
                Log("Task #" << task_log_id << ": execution_start");

                task.time_ms = 0;
                task.nodes_done = 0;
                task.progress = 0.0f;
                return;
            }

            if (type == "execution_success")
            {
                if (j["data"].contains("timestamp") && j["data"]["timestamp"].is_number_integer())
                    task.time_end = j["data"]["timestamp"].get<uint64_t>();
                else task.time_end = tools::dt::from_current_date().time();

                task.time_ms = task.time_end - task.time_start;
                task.nodes_done = m_current->nodes_count;
                task.progress = 1.0f;
                task.result = TaskResult::Success;
                m_current->state = ExecGroup::State::Finished;
                Log("Task #" << task_log_id << ": execution_success. Time: " << task.time_ms << "ms");
                return;
            }

            if (type == "execution_error" || type == "execution_interrupted")
            {
                if (j["data"].contains("timestamp") && j["data"]["timestamp"].is_number_integer())
                    task.time_end = j["data"]["timestamp"].get<uint64_t>();
                else task.time_end = tools::dt::from_current_date().time();

                task.time_ms = task.time_end - task.time_start;
                task.nodes_done = m_current->nodes_count;
                task.progress = 1.0f;
                task.result = (type == "execution_error" ? TaskResult::Failed : TaskResult::Interrupted);
                m_current->state = ExecGroup::State::Finished;

                if (j["data"].contains("exception_message") && j["data"]["exception_message"].is_string())
                {
                    task.err_message = j["data"]["exception_message"];
                    append_err(task.err_full, task.err_message);
                }
                if (j["data"].contains("node_type") && j["data"]["node_type"].is_string())
                    append_err(task.err_full, "Node: " + j["data"]["node_type"].get<std::string>());
                if (j["data"].contains("traceback") && j["data"]["traceback"].is_array())
                {
                    for (const auto& jtrace : j["data"]["traceback"])
                    {
                        if (jtrace.is_string())
                            append_err(task.err_full, jtrace.get<std::string>());
                    }
                }

                iid node = 0;
                if (j["data"].contains("node_id"))
                {
                    if (j["data"]["node_id"].is_number_integer())
                        node = j["data"]["node_id"].get<int32_t>();
                    else if (j["data"]["node_id"].is_string())
                        node = std::strtoul(j["data"]["node_id"].get<std::string>().c_str(), nullptr, 10);

                    if (node != 0)
                        task.err_nodes[node] = {};
                }

                Log("Task #" << task_log_id << ": " << get_result_name(task.result) << ". Err: " << task.err_message);
                return;
            }

            if (m_current->nodes_count > 0 && type == "executing")
            {
                task.nodes_done++;
                const bool has_node = j["data"].contains("node");

                task.current_node = 0;
                if (has_node && j["data"]["node"].is_number_integer())
                    task.current_node = j["data"]["node"].get<int32_t>();
                else if (has_node && j["data"]["node"].is_string())
                    task.current_node = std::strtoul(j["data"]["node"].get<std::string>().c_str(), nullptr, 10);
                else if (has_node && j["data"]["node"].is_null())
                {
                    task.nodes_done = m_current->nodes_count;
                    task.progress = 1.0f;
                    Log("Task #" << task_log_id << ": executing. Progress 100%");
                    return;
                }

                const auto completed_nodes = std::max(0, task.nodes_done - 1);
                const float total_progress = cast::to_float(completed_nodes) / cast::to_float(m_current->nodes_count);
                task.progress = std::clamp(total_progress, 0.0f, 1.0f);
                Log("Task #" << task_log_id << ": executing. Progress " << task.progress << ", curr node " << task.current_node);
                return;
            }

            if (m_current->nodes_count > 0 && type == "progress")
            {
                if (!j["data"].contains("value") || !j["data"].contains("max") || !j["data"].contains("node"))
                    return;
                if (!j["data"]["value"].is_number_integer() || !j["data"]["max"].is_number_integer())
                    return;

                const auto& jnode = j["data"]["node"];
                const auto val = j["data"]["value"].get<int32_t>();
                const auto max = j["data"]["max"].get<int32_t>();
                if (max <= 0)
                    return;

                iid node = 0;
                if (jnode.is_number_integer())
                    node = jnode.get<int32_t>();
                else if (jnode.is_string())
                    node = std::strtoul(jnode.get<std::string>().c_str(), nullptr, 10);

                if (node > 0)
                {
                    const auto completed_nodes = std::max(0, task.nodes_done - 1);
                    const float base_progress = cast::to_float(completed_nodes) / cast::to_float(m_current->nodes_count);
                    float node_progress = cast::to_float(val) / cast::to_float(max);
                    node_progress = std::clamp(node_progress, 0.0f, 1.0f);
                    const float task_weight = 1.0f / cast::to_float(m_current->nodes_count);
                    const float total_progress = base_progress + (node_progress * task_weight);
                    task.progress = std::clamp(total_progress, 0.0f, 1.0f);
                    Log("Task #" << task_log_id << ": progress " << task.progress << ", curr node " << node << "(" << node_progress << ")");
                }
                return;
            }

            if (type == "executed")
            {
                if (!j["data"].contains("output") || !j["data"]["output"].is_object())
                {
                    Log("Task #" << task_log_id << ": executed. No output data");
                    return;
                }

                iid display_node = 0;
                if (j["data"].contains("display_node"))
                {
                    if (j["data"]["display_node"].is_string())
                        display_node = std::strtoul(j["data"]["display_node"].get<std::string>().c_str(), nullptr, 10);
                    else if (j["data"]["display_node"].is_number_integer())
                        display_node = j["data"]["display_node"].get<iid>();
                }

                const auto& joutput = j["data"]["output"];
                for (const auto&[jtype, jarr] : joutput.items())
                {
                    if ((jtype == "images" || jtype == "gifs" || jtype == "audio") && jarr.is_array())
                    {
                        ExecTask::OutputType out_type = ExecTask::OutputType::Unknown;
                        if (jtype == "images")
                            out_type = ExecTask::OutputType::Image;
                        else if (jtype == "gifs")
                            out_type = ExecTask::OutputType::Gif;
                        else // audio
                            out_type = ExecTask::OutputType::Audio;

                        for (auto& jitem : jarr)
                        {
                            ExecTask::Output out;
                            out.type = out_type;
                            out.target_node = display_node;
                            if (jitem.is_object())
                            {
                                if (jitem.contains("filename") && jitem["filename"].is_string())
                                    out.filename = jitem["filename"];
                                if (jitem.contains("subfolder") && jitem["subfolder"].is_string())
                                    out.subfolder = jitem["subfolder"];
                                if (jitem.contains("type") && jitem["type"].is_string())
                                    out.filetype = jitem["type"];
                                if (jitem.contains("format") && jitem["format"].is_string())
                                    out.format = jitem["format"];
                            }

                            out.id = base::GlobalID::random();
                            task.outputs.push_back(std::move(out));
                        }
                    }
                    else if (jtype == "text" && jarr.is_array())
                    {
                        ExecTask::Output out;
                        out.target_node = display_node;
                        out.type = ExecTask::OutputType::Text;
                        for (auto& jitem : jarr)
                        {
                            if (jitem.is_string())
                                out.text += jitem.get<std::string>() + "\r\n";
                        }
                        out.id = base::GlobalID::random();
                        out.ready = true;
                        task.outputs.push_back(std::move(out));
                    }
                    else
                    {
                        ExecTask::Output out;
                        out.target_node = display_node;
                        out.type = ExecTask::OutputType::Unknown;
                        out.id = base::GlobalID::random();
                        task.outputs.push_back(std::move(out));
                    }
                    Log("Task #" << task_log_id << ": executed. Output data type: " << jtype << ". Is format valid: " << jarr.is_array());
                }

                load_outputs(m_current->src_wf, task);
            }
        }
        catch (const std::exception& ex)
        {
            LogExcep("WS message parse ex: " << ex.what());
        }
    };

    g::pFlow->check_ws_message(callback);
}

void QueueManager::load_outputs(wid wf_id, ExecTask& task)
{
    for (auto& out : task.outputs)
    {
        if (ExecTask::OutputType::Audio == out.type || ExecTask::OutputType::Gif == out.type || ExecTask::OutputType::Image == out.type)
        {
            const auto callback = [this, task = task.task_id, id = out.id, wf_id](int32_t result, std::string&& binary)
            {
                LogS("Task #" << task << " load output http result: " << result << ", payload sz " << binary.size());
                if (result != HTTP_RESP_OK)
                    binary.clear();
                load_output_data(wf_id, task, id, std::move(binary));
            };

            const auto request = fmt::format(comfyui_http::view_, tools::str::url_encode(out.filename), tools::str::url_encode(out.subfolder), tools::str::url_encode(out.filetype));
            g::pFlow->http_get(HttpRequestor::Queue_Output, get_alive_callback(), request.c_str(), callback);
        }
        else if (ExecTask::OutputType::Text == out.type && out.target_node != 0)
            g::pFlow->custom_widgets().add_output(wf_id, out.target_node, out.type, out.text, "", 0);
    }
}

void QueueManager::load_output_data(wid wf_id, uint64_t task_id, uint64_t out_id, const std::string&& binary)
{
    ExecTask* task = nullptr;
    if (m_current != nullptr && m_current->task.task_id == task_id)
        task = &m_current->task;

    if (nullptr == task)
    {
        for (auto& g : m_queue_past)
        {
            for (auto& t : g.completed)
            {
                if (t.task_id == task_id)
                {
                    task = &t;
                    break;
                }
            }
            if (task != nullptr)
                break;
        }
    }
    LogRAssert(task, "Output task " << task_id << " not found", );

    for (auto& out : task->outputs)
    {
        if (out.id == out_id)
        {
            if (binary.empty() || out.filename.empty())
            {
                out.exists = false;
                out.ready = true;
                continue;
            }

            const auto temp_filename = fs::get_temp_filepath(tools::path::path_to_string(std::filesystem::path(out.filename).extension()));
            if (temp_filename.empty())
                continue;
            if (!tools::path::make_file(temp_filename, false, binary))
                continue;

            out.binary_path = tools::path::path_to_string(temp_filename);
            if (ExecTask::OutputType::Image == out.type)
                out.media_id = g::pTextures->load_image_from_path(out.binary_path, QUEUE_IMAGE_RECT, QUEUE_IMAGE_RECT, QUEUE_IMAGE_LIFETIME);

            if (out.target_node != 0)
                g::pFlow->custom_widgets().add_output(wf_id, out.target_node, out.type, out.binary_path, out.filename, out.media_id);

            out.exists = true;
            out.ready = true;
            Log("Task #" << get_log_task_id(*task) << ". Output saved to " << out.binary_path);
            break;
        }
    }
}

void QueueManager::check_current_task_is_done()
{
    m_lost = false;
    const auto history_callback = [this](int32_t result, std::string&& binary)
    {
        if (result != HTTP_RESP_OK || nullptr == m_current) [[unlikely]]
        {
            if (m_current != nullptr)
            {
                m_current->state = ExecGroup::State::Finished;
                m_current->task.result = TaskResult::Failed;
            }
            go_next_task();
            return;
        }

        try
        {
            const auto j = o_json::parse(binary);
            if (j.is_object())
            {
                for (auto& [prompt_id, jdatas] : j.items())
                {
                    (void)jdatas;
                    if (m_current->task.prompt_id == prompt_id)
                    {
                        m_current->state = ExecGroup::State::Finished;
                        m_current->task.result = TaskResult::Lost;
                        go_next_task();
                        return;
                    }
                }
            }

            m_current->state = ExecGroup::State::Finished;
            m_current->task.result = TaskResult::Failed;
            go_next_task();
        }
        catch (const std::exception& ex)
        {
            LogExcepS("History parsing err " << ex.what());
        }
    };

    const auto history_request = fmt::format(comfyui_http::history_, 100);
    g::pFlow->http_get(HttpRequestor::Queue_History, get_alive_callback(), history_request.c_str(), history_callback);
}

} // namespace mngr
