#include "ui/panels/panel_queue.h"
#include "ui/helpers/textures.h"
#include "ui/helpers/audio_control.h"
#include "ui/helpers/video_control.h"
#include "core/managers/queue_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/db_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/serializer/wf_serializers.h"
#include "core/base/comfyui_web_paths.h"

namespace ui
{

#ifdef _WIN32
const char* codec_error_msg = 
    "Unable to decode video. The required codec is missing from your operating system.\n\n"
    "Please install the corresponding package from the Microsoft Store based on your video format:\n"
    "  • For H.265 / HEVC: Install \"HEVC Video Extensions\"\n"
    "  • For AV1: Install \"AV1 Video Extension\"\n"
    "  • For WebM / VP9: Install \"Web Media Extensions\"";
#else
const char* codec_error_msg = "Unable to decode video";
#endif

PanelQueue::PanelQueue()
    : PanelBase(DlgObj::PanelQueue, "Queue")
{}

PanelQueue::~PanelQueue() noexcept = default;

void PanelQueue::init()
{
    PanelBase::init();

    m_load_history_stop = std::make_shared<std::atomic_bool>();

    m_favorited = g::pOptsManager->items_history_favs_load();
    create_toolbar();

    m_history_tasks = g::pDbManager->history_load_all();
    for (auto& t : m_history_tasks)
    {
        m_hidden.insert(t.group_id);
        t.favorited = m_favorited.has_item(t.prompt_id);
    }
}

void PanelQueue::init_history()
{
    if (!g::pFlow->get_online() || m_force_reload_history)
    {
        m_force_reload_history = false;
        m_do_reload_history.store(true, std::memory_order_relaxed);
        m_load_history_stop->store(true, std::memory_order_relaxed);
        m_final_history_pickup_done = m_ready_pickup_history = false;
    }
    else if (m_do_reload_history.load(std::memory_order_relaxed))
    {
        if (m_history_loading_active.load(std::memory_order_relaxed))
            return;

        m_do_reload_history.store(false, std::memory_order_relaxed);
        m_load_history_stop->store(false, std::memory_order_relaxed);

        std::erase_if(m_history_tasks,
            [this](const mngr::ExecTask& t)
        {
            if (!t.loaded_from_db)
                m_hidden.erase(t.task_id);
            return !t.loaded_from_db;
        });

        m_history_tasks.reserve(m_history_tasks.size() + g::pOptsManager->queue_history_num_load());

        m_history_parsed_async.clear();
        m_history_parsed_temp.clear();

        m_history_loading_active.store(true, std::memory_order_relaxed);
        m_ready_pickup_history = true;
        std::thread(&PanelQueue::load_history, this).detach();
    }
}

void PanelQueue::stop_all_threads() noexcept
{
    m_do_reload_history = false;
    *m_load_history_stop = true;
}

DlgCode PanelQueue::show(bool& focused)
{
    auto queue = g::pQueueManager;

    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        if (m_history_tasks.empty() && queue->is_external_task() == 0 && queue->get_current_group() == nullptr && queue->get_past_tasks().empty() && queue->get_next_tasks().empty())
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Queue.",
                {
                    "Monitor and manage your",
                    "execution queue.",
                    "",
                    "Access finished workflows",
                    "and media outputs.",
                },
                {
                    "The queue and history",
                    "are currently empty.",
                    "",
                    "Queue a prompt from the Workflow",
                    "panel to see your tasks here."
                }
            );
        }
        else [[likely]]
        {
            draw_toolbar();
            init_history();

            if (ImGui::BeginChild(DlgObjID[cast::to_szt(DlgObj::PanelQueue_ItemsList)], ImVec2(ImGui::GetContentRegionAvail().x, 0), false, (m_fix_panel_scroll ? ImGuiWindowFlags_AlwaysVerticalScrollbar : 0)))
            {
                ImGui::PushID("Current");
                show_current_task();
                ImGui::PopID();

                ImGui::PushID("Next");
                show_next_tasks();
                ImGui::PopID();

                ImGui::PushID("Favs");
                show_favs_tasks();
                ImGui::PopID();

                ImGui::PushID("Past");
                show_past_tasks();
                ImGui::PopID();

                ImGui::PushID("Loaded");
                show_history_tasks();
                ImGui::PopID();
            }

            ImGuiEx::toast_show(m_toast);
            ImGui::EndChild();
        }
    }

    return end_panel(opened);
}

void PanelQueue::create_toolbar()
{
    clear_toolbar();

    // left
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Empty;
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_PAUSE;
        el.tooltip = "Pause";
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_pause(el); };
        el.on_click = [this](PanelToolbar::Element& el) noexcept { on_pause(el); };
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_X;
        el.tooltip = "Terminate current task";
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_terminate_current(el); };
        el.on_click = [this](PanelToolbar::Element& el) { on_terminate_current(el); };
        add_toolbar_element(std::move(el), true);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_SQUARE;
        el.tooltip = "Drop all tasks";
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_remove_all(el); };
        el.on_click = [this](PanelToolbar::Element& el) noexcept { on_remove_all(el); };
        add_toolbar_element(std::move(el), true);
    }

    // right
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_STAR;
        el.tooltip = "Hide favorited";
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_toggle_favorited(el); };
        el.on_click = [this](PanelToolbar::Element& el) noexcept { on_toggle_favorited(el); };
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_HISTORY;
        el.tooltip = "Hide completed";
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_toggle_past(el); };
        el.on_click = [this](PanelToolbar::Element& el) noexcept { on_toggle_past(el); };
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Empty;
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_LIST_CHEVRONS_UP_DOWN;
        el.tooltip = "Expand all history";
        el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_toggle_history(el); };
        el.on_click = [this](PanelToolbar::Element& el) { on_toggle_history(el); };
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_CLOUD_DOWNLOAD;
        el.tooltip = "Reload history";
        el.on_click = [this](PanelToolbar::Element& el) noexcept { on_reload_history(el); };
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Empty;
        add_toolbar_element(std::move(el), false);
    }

    finalize_toolbar();
}

int32_t PanelQueue::show_tasks_num(int32_t curr) noexcept
{
    if (curr < 0) curr = -1;
    int32_t imgui_idx = curr + 1;
    if (ImGui::Combo("", &imgui_idx,
        [](void* /*data*/, int idx, const char** out_text) noexcept
        {
            if (0 == idx) [[unlikely]]
            {
                *out_text = "Infinite";
                return true;
            }

            static char buffer[12];
            auto[ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer) - 1, idx - 1);
            *ptr = '\0';

            *out_text = buffer;
            return true;

        }, nullptr, QUEUE_MAX_RUNS + 2))
    {
        return imgui_idx - 1;
    }

    return curr;
}

void PanelQueue::show_current_task()
{
    m_visible_curr = false;

    if (g::pQueueManager->is_external_task() > 0)
    {
        m_visible_curr = true;
        show_external_task();
        return;
    }

    auto group = g::pQueueManager->get_current_group();
    if (nullptr == group)
        return;

    auto& t = group->task;
    if (!t.active)
        return;

    ImGuiEx::Spacing(2);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), 0.0f, "Processing");
    ImGuiEx::Spacing(1);
    m_visible_curr = true;


    const auto align_and_set_pos = [this]()
    {
        m_curr_pos_x = std::max(m_curr_pos_x, ImGui::GetCursorPosX());
        ImGui::SetCursorPosX(m_curr_pos_x);
    };

    ImGui::TextUnformatted("Workflow:");
    ImGui::SameLine();
    align_and_set_pos();
    if (ImGuiEx::UrlButton(group->wf_name.c_str(), "Activate workflow", IM_COL32_WHITE, g::pOptsManager->color_load(AppColor::SelectedItem), false))
        g::pFlow->wf_activate(group->src_wf);

    ImGui::TextUnformatted("Progress:");

    const bool dead_task = g::pQueueManager->is_dead_task();

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, g::pOptsManager->color_load(dead_task ? AppColor::DisconnectedMenuBarBg : AppColor::OptSeparator));
    ImGui::SameLine();
    align_and_set_pos();
    ImGui::ProgressBar(group->task.progress, ImVec2(-FLT_MIN, 0.0f));

    if (dead_task)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::DisconnectedMenuBarBg));
        ImGui::TextUnformatted("Task is taking longer than expected");
    }
    ImGui::PopStyleColor(dead_task ? 2 : 1);

    ImGui::TextUnformatted("Node:");
    if (t.current_node != 0)
    {
        const auto* node = group->wf->get_node_by_instance(t.current_node);
        if (node != nullptr)
        {
            ImGui::SameLine();
            align_and_set_pos();
            ImGui::TextUnformatted(node->get_name().c_str(), node->get_name().c_str() + node->get_name().size());
        }
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Runs: ");
    ImGui::SameLine();
    align_and_set_pos();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    const auto new_num = show_tasks_num(group->tasks_num);
    if (new_num != group->tasks_num)
        g::pQueueManager->modify_group(group->group_id, new_num);
}

void PanelQueue::show_external_task()
{
    const mngr::ExecTask* task = nullptr;
    auto group = g::pQueueManager->get_current_group();
    if (group != nullptr && group->task.active)
        task = &group->task;

    ImGuiEx::Spacing(2);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), 0.0f, "Processing");
    ImGuiEx::Spacing(1);

    const bool dead_task = g::pQueueManager->is_dead_task();
    if (dead_task)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::DisconnectedMenuBarBg));
        ImGui::TextUnformatted("External task is taking longer than expected");
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, g::pOptsManager->color_load(AppColor::DisconnectedMenuBarBg));
    }
    else
    {
        ImGui::TextUnformatted("External task is running");
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, g::pOptsManager->color_load(AppColor::OptSeparator));
    }
    ImGuiEx::IndeterminateProgressBar(ImVec2(-1.0f, 0.0f));
    ImGui::PopStyleColor(dead_task ? 2 : 1);
    ImGui::Text("Queue remaining: %d", g::pQueueManager->is_external_task());

    if (task != nullptr)
    {
        ImGuiEx::Spacing(1);
        ImGui::TextUnformatted("Queued:");
        ImGui::SameLine();
        if (ImGuiEx::UrlButton(group->wf_name.c_str(), "Activate workflow", IM_COL32_WHITE, g::pOptsManager->color_load(AppColor::SelectedItem), false))
            g::pFlow->wf_activate(group->src_wf);
    }
}

void PanelQueue::show_next_tasks()
{
    m_visible_pend = false;
    auto& next = g::pQueueManager->get_next_tasks();
    if (next.empty())
        return;

    ImGuiEx::Spacing(m_visible_curr ? 4 : 2);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), 0.0f, "Pending Queue");
    ImGuiEx::Spacing(1);
    m_visible_pend = true;


    std::pair<int64_t, int64_t> swap = { 0, 0 };

    for (auto& g : next)
    {
        ImGui::PushID(cast::to_void(g.group_id));

        const auto start_pos = ImGui::GetCursorPos();
        ImGui::AlignTextToFramePadding();
        ImGui::Selectable("##DND_REORDER_QUEUE", false, int32_t(ImGuiSelectableFlags_SpanAvailWidth) | int32_t(ImGuiSelectableFlags_AllowItemOverlap));

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) [[unlikely]]
        {
            ImGui::SetDragDropPayload(DND_REORDER_QUEUE, &g.group_id, sizeof(uint64_t));
            ImGui::TextUnformatted(g::pFlow->get_panel_icon(DlgObj::PanelWf));
            ImGui::SameLine();
            ImGui::TextUnformatted(g.wf_name.c_str(), g.wf_name.c_str() + g.wf_name.size());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) [[unlikely]]
        {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DND_REORDER_QUEUE);
            if (payload != nullptr && payload->DataSize == sizeof(uint64_t))
            {
                const auto group_id = *(const uint64_t*)payload->Data;
                swap = { group_id, g.group_id };
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SetCursorPos(start_pos);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(g::pFlow->get_panel_icon(DlgObj::PanelWf));

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(g.wf_name.c_str(), g.wf_name.c_str() + g.wf_name.size());

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 100);
        ImGui::SetNextItemWidth(100);
        const auto new_num = show_tasks_num(g.tasks_num);
        if (new_num != g.tasks_num)
            g::pQueueManager->modify_group(g.group_id, new_num);

        ImGui::PopID();
    }

    if (swap.first != swap.second)
        g::pQueueManager->swap_groups(swap.first, swap.second);
}

void PanelQueue::show_favs_tasks()
{
    m_visible_favs = false;
    if (m_favorited.count() == 0)
        return;

    auto& past = g::pQueueManager->get_past_tasks();
    if (past.empty() && m_history_tasks.empty())
        return;

    const auto show_separator = [this]()
    {
        if (m_visible_favs)
            return;

        ImGuiEx::Spacing(m_visible_curr || m_visible_pend ? 4 : 2);
        ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), 0.0f, "Favorited");
        ImGuiEx::Spacing(1);
        m_visible_favs = true;
    };

    const auto is_favorited = [this](const mngr::ExecTask& task)
    {
        if (task.favorited.has_value())
            return task.favorited.value();
        return false;
    };

    for (auto it = past.rbegin(); it != past.rend(); ++it)
    {
        auto& g = *it;
        for (auto& t : g.completed)
        {
            if (is_favorited(t))
            {
                show_separator();
                if (m_toggled_favs)
                    show_past_task(g.group_id, &t, g.wf_name);
            }
        }
    }

    for (auto& t : m_history_tasks)
    {
        if (is_favorited(t))
        {
            show_separator();
            if (m_toggled_favs)
                show_past_task(0, &t, t.time_start_s);
        }
    }
}

void PanelQueue::show_past_tasks()
{
    m_visible_past = false;
    auto& past = g::pQueueManager->get_past_tasks();
    if (past.empty())
        return;

    ImGuiEx::Spacing(m_visible_curr || m_visible_pend || m_visible_favs ? 4 : 2);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), 0.0f, "Completed");
    ImGuiEx::Spacing(1);
    m_visible_past = true;

    if (!m_toggled_past)
        return;

    const float indent_w = ImGui::CalcTextSize(g::pFlow->get_panel_icon(DlgObj::PanelWf)).x;
    int32_t id_guard = 1;
    size_t count = 0;

    if (ImGui::GetScrollY() < 5.0f && m_current_completed_displays > m_completed_display_limit)
    {
        m_current_completed_displays = m_completed_display_limit;
    }

    for (auto it = past.rbegin(); it != past.rend(); ++it)
    {
        if (count > m_current_history_displays)
            break;

        auto& g = *it;
        ImGui::PushID(id_guard++);
        ImGui::PushID(cast::to_void(g.group_id));

        ImGui::TextUnformatted(g::pFlow->get_panel_icon(DlgObj::PanelWf));
        ImGui::SameLine();

        bool hidden = m_hidden.contains(g.group_id);
        if (ImGuiEx::UrlButton(g.wf_name.c_str(), hidden ? "Show details" : "Hide details", IM_COL32_WHITE, g::pOptsManager->color_load(AppColor::SelectedItem), false))
        {
            if (hidden) m_hidden.erase(g.group_id);
            else m_hidden.insert(g.group_id);
            hidden = !hidden;
        }

        if (hidden)
        {
            ImGui::PopID();
            ImGui::PopID();
            continue;
        }

        ImGui::Indent(indent_w);

        for (auto& t : g.completed)
        {
            count++;
            if (count++ > m_current_completed_displays)
                break;

            show_past_task(g.group_id, &t);
        }

        ImGui::Unindent(indent_w);
        ImGui::PopID();
        ImGui::PopID();
    }

    if (count > m_current_completed_displays)
    {
        ImGuiEx::Spacing(2);
        if (ImGui::Button("Load more...", ImVec2(-FLT_MIN, 30)))
        {
            m_current_completed_displays += m_completed_display_limit;
        }
    }
}

void PanelQueue::show_history_tasks()
{
    pickup_parsed_history();
    if (m_history_tasks.empty())
        return;

    ImGuiEx::Spacing(m_visible_curr || m_visible_pend || m_visible_favs || m_visible_past ? 4 : 2);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), 0.0f, "History");
    ImGuiEx::Spacing(1);

    const float indent_w = ImGui::CalcTextSize(g::pFlow->get_panel_icon(DlgObj::PanelWf)).x;
    size_t count = 0;

    if (ImGui::GetScrollY() < 5.0f && m_current_history_displays > m_history_display_limit)
    {
        m_current_history_displays = m_history_display_limit;
    }

    for (auto& task : m_history_tasks)
    {
        if (count++ > m_current_history_displays)
            break;

        ImGui::PushID(task.prompt_id.c_str());

        ImGui::TextUnformatted(g::pFlow->get_panel_icon(DlgObj::PanelWf));
        ImGui::SameLine();

        bool hidden = m_hidden.contains(task.group_id);
        if (ImGuiEx::UrlButton(task.time_start_s.c_str(), hidden ? "Show details" : "Hide details", IM_COL32_WHITE, g::pOptsManager->color_load(AppColor::SelectedItem), false))
        {
            if (hidden) m_hidden.erase(task.group_id);
            else m_hidden.insert(task.group_id);
            hidden = !hidden;
        }

        if (hidden)
        {
            ImGui::PopID();
            continue;
        }

        ImGui::Indent(indent_w);
        show_past_task(0, &task);
        ImGui::Unindent(indent_w);
        ImGui::PopID();
    }

    if (count > m_current_history_displays)
    {
        ImGuiEx::Spacing(2);
        if (ImGui::Button("Load more...", ImVec2(-FLT_MIN, 30)))
        {
            m_current_history_displays += m_history_display_limit;
        }
    }
}

void PanelQueue::show_past_task(uint64_t group_id, mngr::ExecTask* task, const std::string& wf_name /*= ""*/)
{
    const auto align_and_set_pos2 = [this]() noexcept
    {
        m_curr_pos2_x = std::max(m_curr_pos2_x, ImGui::GetCursorPosX());
        ImGui::SetCursorPosX(m_curr_pos2_x);
    };

    const auto is_favorited = [this](mngr::ExecTask* task)
    {
        if (task->favorited.has_value())
            return task->favorited.value();

        task->favorited = m_favorited.has_item(task->prompt_id);
        return task->favorited.value();
    };

    ImGui::PushID(cast::to_void(task->task_id));

    const bool favorited = is_favorited(task);
    const char* icon = (favorited ? ICON_LC_STAR : ICON_LC_PLAY);
    const char* tt = (favorited ? "Unfavorite run" : "Favorite run");
    const AppColor clr = (favorited ? AppColor::Starred : AppColor::OptSeparator);
    if (ImGuiEx::UrlButton(icon, tt, g::pOptsManager->color_load(clr), g::pOptsManager->color_load(AppColor::SelectedItem), false)) [[unlikely]]
    {
        task->favorited = !task->favorited.value();
        m_favorited.toggle_item(task->prompt_id);
        g::pOptsManager->items_history_favs_save(m_favorited);
    }

    char run_no_txt[15] = {};
    auto end_ptr = fmt::format_to_n(run_no_txt, sizeof(run_no_txt), FMT_STRING("Run {}."), task->run_no).out;
    *end_ptr = 0;
    ImGui::SameLine();
    const float off1 = ImGui::GetCursorPosX();
    ImGui::TextUnformatted(run_no_txt);

    ImGui::SameLine();
    align_and_set_pos2();

    const auto disable = !g::pFlow->get_nodes_ready() || task->flow->empty();
    if (disable) ImGui::BeginDisabled();
    const char* wf_text = (wf_name.empty() ? "Open workflow" : wf_name.c_str());
    if (ImGuiEx::UrlButton(wf_text, "Create and open workflow for this run", IM_COL32_WHITE, g::pOptsManager->color_load(AppColor::SelectedItem), false)) [[unlikely]]
    {
        if (!task->loaded_from_server && !task->loaded_from_db)
            g::pQueueManager->create_from_past_task(group_id, task->task_id);
        else
        {
            const auto& blob = task->flow;
            const std::string new_wf = (wf_name.empty() ? task->prompt_id : wf_name.c_str());
            auto& data = g::pWfsManager->create_temp(blob, "Q:" + std::to_string(task->run_no) + ":" + new_wf, TEMP_LABEL_COMPLETED);
            task->open_id = data.id;
            g::pFlow->wf_activate(data.id);
        }
    }
    if (disable) ImGui::EndDisabled();

    if (!task->loaded_from_server && !task->loaded_from_db)
    {
        ImGui::SetCursorPosX(off1);
        ImGui::TextUnformatted("Started:");
        ImGui::SameLine();
        align_and_set_pos2();
        ImGui::TextUnformatted(task->time_start_s.c_str(), task->time_start_s.c_str() + task->time_start_s.size());
    }

    ImGui::SetCursorPosX(off1);
    ImGui::TextUnformatted("Time:");
    ImGui::SameLine();
    align_and_set_pos2();
    ImGui::Text("%.1fs", task->time_ms / 1000.0f);

    ImGui::SetCursorPosX(off1);
    ImGui::TextUnformatted("Status:");
    ImGui::SameLine();
    align_and_set_pos2();
    if (TaskResult::Success == task->result)
        ImGui::TextUnformatted("Completed");
    else if (TaskResult::Lost == task->result)
        ImGui::TextUnformatted("Check history");
    else if (TaskResult::Interrupted == task->result)
        ImGui::TextUnformatted("Interrupted");
    else
        ImGui::TextUnformatted("Failed");

    bool hidden = m_hidden.contains(task->task_id);

    const float window_width = ImGui::GetWindowWidth();
    const float padding_x = ImGui::GetStyle().WindowPadding.x;
    float avail = window_width - (padding_x * 2.0f) - ImGui::GetStyle().ScrollbarSize; // fix feedback loop

    if (task->result != TaskResult::Success) [[unlikely]]
    {
        ImGui::SetCursorPosX(off1);
        if (ImGuiEx::UrlButton("Error:", hidden ? "Show error details" : "Hide error details", IM_COL32_WHITE, g::pOptsManager->color_load(AppColor::SelectedItem), false))
        {
            if (hidden) m_hidden.erase(task->task_id);
            else m_hidden.insert(task->task_id);
            hidden = !hidden;
        }

        if (!hidden)
        {
            ImGui::SetCursorPosX(off1);
            ImGui::SetNextItemWidth(avail);
            ImGuiEx::InputTextMultiline("", &task->err_full, ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly);
        }
    }
    else if (!task->outputs.empty())
    {
        ImGui::SetCursorPosX(off1);
        if (ImGuiEx::UrlButton("Outputs:", hidden ? "Show output details" : "Hide output details", IM_COL32_WHITE, g::pOptsManager->color_load(AppColor::SelectedItem), false))
        {
            if (hidden) m_hidden.erase(task->task_id);
            else m_hidden.insert(task->task_id);
            hidden = !hidden;
        }

        if (!hidden)
        {
            int32_t o_pushid = 0;
            for (auto it = task->outputs.rbegin(); it != task->outputs.rend(); ++it)
            {
                auto& o = *it;
                if (!o.ready && !task->loaded_from_server && !task->loaded_from_db)
                    continue;

                float height = 0.0f;

                switch (o.type)
                {
                case mngr::ExecTask::OutputType::Text:
                    height = QUEUE_TEXT_RECT;
                    break;
                case mngr::ExecTask::OutputType::Gif:
                    height = QUEUE_IMAGE_RECT;
                    break;
                case mngr::ExecTask::OutputType::Image:
                    height = QUEUE_IMAGE_RECT;
                    break;
                case mngr::ExecTask::OutputType::Audio:
                    height = QUEUE_AUDIO_RECT;
                    break;
                default:
                    ImGui::SetCursorPosX(off1);
                    ImGui::TextUnformatted("Unknown output format");
                    continue;
                }

                avail = std::min(avail, cast::to_float(QUEUE_IMAGE_RECT));
                if (mngr::ExecTask::OutputType::Image == o.type || mngr::ExecTask::OutputType::Gif == o.type) // must be square
                    height = std::min(avail, height);
                auto rect = ImVec2(avail, height);
                if (!ImGui::IsRectVisible(rect))
                {
                    if (mngr::ExecTask::OutputType::Gif == o.type)
                        rect.y += QUEUE_AUDIO_RECT;
                    ImGui::Dummy(rect);
                    continue;
                }

                ImGui::PushID(++o_pushid);
                ImGui::SetCursorPosX(off1);

                show_asset_info(o, task->outputs.size() > 1 ? o_pushid : 0);

                if (mngr::ExecTask::OutputType::Text == o.type)
                {
                    ImGuiEx::InputTextMultiline("", &o.text, rect, ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopID();
                    continue;
                }

                if (o.media_valid.has_value() && !o.media_valid.value())
                {
                    ImGui::TextUnformatted("Media: corrupted or not supported");
                    ImGui::PopID();
                    continue;
                }

                bool show_stub = true;
                if (!o.ready && g::pFlow->get_online())
                {
                    if (!o.preparing)
                    {
                        const auto data_callback = [this, task = task->task_id, id = o.id](int32_t result, std::string&& binary)
                        {
                            if (result != HTTP_RESP_OK)
                            {
                                LogS("Task " << task << ": load output failed");
                                load_output_data(task, id, "");
                            }
                            else
                            {
                                load_output_data(task, id, std::move(binary));
                            }
                        };

                        const auto data_request = fmt::format(comfyui_http::view_, tools::str::url_encode(o.filename), tools::str::url_encode(o.subfolder), tools::str::url_encode(o.filetype));
                        g::pFlow->http_get("QPanel/Output", get_alive_callback(), data_request.c_str(), data_callback);
                        o.preparing = true;
                    }
                }
                else if (!o.exists)
                {
                    ImGui::TextUnformatted("Media: no data");
                    show_stub = false;
                }
                else
                {
                    switch (o.type)
                    {
                    case mngr::ExecTask::OutputType::Gif:
                        show_video(o, rect, show_stub);
                        break;
                    case mngr::ExecTask::OutputType::Audio:
                        show_audio(o, rect, show_stub);
                        break;
                    case mngr::ExecTask::OutputType::Image:
                        show_image(o, rect, show_stub);
                        break;
                    }
                }

                if (show_stub)
                {
                    if (mngr::ExecTask::OutputType::Gif == o.type)
                        rect.y += QUEUE_AUDIO_RECT;
                    ImGui::Button("Loading...", rect);
                }

                ImGui::PopID();
            }
        }
    }

    ImGui::PopID();
}

void PanelQueue::show_asset_info(const mngr::ExecTask::Output& output, size_t out_idx)
{
    static char buffer[100] = {};
    const auto pos = ImGui::GetCursorPosX();

    if (out_idx > 0)
    {
        const auto old_padding = ImGui::GetStyle().SeparatorTextPadding;
        const auto old_thickness = ImGui::GetStyle().SeparatorTextBorderSize;
        ImGui::GetStyle().SeparatorTextPadding = ImVec2(0.0f, 0.0f);
        ImGui::GetStyle().SeparatorTextBorderSize = 1.0f;

        ImVec4 sep_color = ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::OptSeparator));
        sep_color.w = 0.3f;
        ImGui::PushStyleColor(ImGuiCol_Separator, sep_color);
        ImGui::PushStyleColor(ImGuiCol_Text, sep_color);

        *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("{}"), out_idx).out = 0;
        ImGui::SeparatorText(buffer);

        ImGui::PopStyleColor(2);
        ImGui::GetStyle().SeparatorTextPadding = old_padding;
        ImGui::GetStyle().SeparatorTextBorderSize = old_thickness;

        ImGui::SetCursorPosX(pos);
    }

    if (mngr::ExecTask::OutputType::Text == output.type)
    {
        if (!output.text.empty())
        {
            *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING(ICON_LC_CLIPBOARD_COPY " {} bytes"), output.text.size()).out = 0;
            if (ImGuiEx::UrlButton(buffer, "Copy to clipboard",
                g::pOptsManager->color_load(AppColor::ActivityBarInactive),
                g::pOptsManager->color_load(AppColor::SelectedItem), false))
            {
                ImGui::SetClipboardText(output.text.c_str());
                m_toast = ImGuiEx::toast_prepare("Copied", g::pOptsManager->color_load(AppColor::SelectedItem));
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::ActivityBarInactive));
            ImGui::TextUnformatted("0 bytes");
            ImGui::PopStyleColor();
        }
    }
    else [[likely]]
    {
        const bool is_audio = (mngr::ExecTask::OutputType::Audio == output.type);
        const bool is_video = (mngr::ExecTask::OutputType::Gif == output.type);
        const bool is_image = (mngr::ExecTask::OutputType::Image == output.type);        

        if (!output.binary_path.empty()) [[likely]]
        {
            if (ImGuiEx::UrlButton(ICON_LC_COPY, "Copy file...",
                    g::pOptsManager->color_load(AppColor::ActivityBarInactive),
                    g::pOptsManager->color_load(AppColor::SelectedItem), false))
            {
                tools::path::FdSelectResultSg callback = [src = output.binary_path, pos = ImGui::GetWindowPos(), size = ImGui::GetWindowSize(), this](bool result, std::filesystem::path&& path)
                {
                    if (result && tools::path::exists(src))
                    {
                        tools::path::copy(src, path, true, false);
                    }
                };

                const std::filesystem::path filename = output.filename;
                const auto ext = filename.extension().wstring();
                const wchar_t* type = (is_audio ? L"Audio" : (is_video ? L"Video" : L"Image"));
                const auto mask1 = fmt::format(FMT_STRING(L"{} (*{})"), type, ext);
                const auto mask2 = fmt::format(FMT_STRING(L"*{}"), ext);
                tools::path::select_file_save({ { mask1, mask2 } }, output.filename, callback);
            }

            ImGui::SameLine();
            if (ImGuiEx::UrlButton(output.filename.c_str(), "Open in default app",
                    g::pOptsManager->color_load(AppColor::ActivityBarInactive),
                    g::pOptsManager->color_load(AppColor::SelectedItem), false))
            {
                tools::path::open_file(output.binary_path);
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::ActivityBarInactive));
            ImGui::TextUnformatted(output.filename.c_str());
            ImGui::PopStyleColor();
        }

        if (output.media_info_valid.has_value() && output.media_info_valid.value())
        {
            ImGui::SetCursorPosX(pos);
            if (is_image)
            {
                *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("w: {}px, h: {}px"), output.image_info.width, output.image_info.height).out = 0;
            }
            else if (is_audio)
            {
                *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("d: {:.3f}s, sr: {}Hz, ch: {}"), output.audio_info.duration_sec, output.audio_info.sample_rate, output.audio_info.channels).out = 0;
            }
            else
            {
                *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("w: {}px, h: {}px, fps: {}, d: {:.3f}s"), output.video_info.width, output.video_info.height, output.video_info.fps, output.video_info.duration_sec).out = 0;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::ActivityBarInactive));
            ImGui::TextUnformatted(buffer);
            ImGui::PopStyleColor();
        }
    }

    ImGui::SetCursorPosX(pos);
}

void PanelQueue::show_image(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub)
{
    if (!g::pTextures->get_image_alive(output.media_id))
    {
        if (tools::path::exists(output.binary_path))
            output.media_id = g::pTextures->load_image(output.binary_path.c_str(), QUEUE_IMAGE_RECT, QUEUE_IMAGE_RECT, QUEUE_IMAGE_LIFETIME);
    }
    else [[likely]]
    {
        const auto textures = g::pTextures->get_image(output.media_id, output.media_valid);        
        if (textures != nullptr)
        {
            if (0 == output.image_info.width || 0 == output.image_info.height)
            {
                g::pTextures->get_original_image_size(output.media_id, output.image_info.width, output.image_info.height);
                output.media_info_valid = output.image_info.width > 0 && output.image_info.height > 0;
            }

            if (output.image_info.width > 0 && output.image_info.height > 0 && output.image_info.width != output.image_info.height)
            {
                int32_t res_x, res_y;
                tools::img::scale(output.image_info.width, output.image_info.height, cast::to_i32(rect.x), cast::to_i32(rect.y), res_x, res_y);
                rect.x = cast::to_float(res_x);
                rect.y = cast::to_float(res_y);
            }

            ImGui::Image(textures, rect);
            if (tools::path::exists(output.binary_path))
            {
                ImGuiEx::TooltipOnLastControl([out = &output]()
                {
                    if (g::pTextures->get_image_alive(out->media_id_tt))
                    {
                        float w, h;
                        std::optional<bool> valid;
                        const auto textures = g::pTextures->get_image(out->media_id_tt, valid, &w, &h);
                        if (textures != nullptr)
                            ImGui::Image(textures, ImVec2(w, h));
                        else if (valid.has_value() && !valid)
                            ImGui::TextUnformatted("Can't render image data");
                    }
                    else
                    {
                        const ImGuiViewport* viewport = ImGui::GetMainViewport();
                        out->media_id_tt = g::pTextures->load_image(out->binary_path.c_str(), cast::to_i32(viewport->WorkSize.x), cast::to_i32(viewport->WorkSize.y), QUEUE_TT_IMAGE_LIFETIME);
                    }
                }, ImGuiHoveredFlags_DelayNormal);
            }
            show_stub = false;
        }
    }
}

namespace
{
const char* format_timeline(char buffer[32], float duration, float pos)
{
    const auto get_times = [](const float time, int32_t& h, int32_t& m, int32_t& s, int32_t& ms) noexcept
    {
        s = cast::to_i32(time);
        ms = cast::to_i32((time - s) * 1000.0f);
        h = s / 3600;
        m = (s % 3600) / 60;
        s = s % 60;
    };

    int32_t total_h, total_m, total_s, total_ms;
    get_times(duration, total_h, total_m, total_s, total_ms);

    int32_t curr_h, curr_m, curr_s, curr_ms;
    get_times(pos, curr_h, curr_m, curr_s, curr_ms);

    if (total_h > 0)
        *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("{:02}:{:02}:{:02}.{:03}"), curr_h, curr_m, curr_s, curr_ms).out = 0;
    else if (total_m > 0)
        *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("{:02}:{:02}.{:03}"), curr_m, curr_s, curr_ms).out = 0;
    else
        *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("{:02}.{:03}"), curr_s, curr_ms).out = 0;

    return buffer;
}
} // namespace

void PanelQueue::show_audio(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub)
{
    show_stub = false;

    bool is_playing = false;
    float audio_pos = 0.0f;
    const bool first_appear = (0 == output.media_id);
    if (first_appear)
    {
        output.audio_info = g::pAudioControl->get_info(output.binary_path, output.media_valid);
        output.media_info_valid = output.media_valid;
        output.media_id = -1;
        if (output.media_valid.has_value() && !output.media_valid.value())
            return;
    }
    else
    {
        is_playing = g::pAudioControl->get_is_playing(output.media_id);
        audio_pos = g::pAudioControl->get_position(output.media_id);
    }

    const auto load = [&output, &show_stub]()
    {
        output.media_id = g::pAudioControl->load(output.binary_path);
        if (0 == output.media_id)
        {
            output.media_valid = false;
            show_stub = false;
            return false;
        }
        return true;
    };

    const auto asset_pos = ImGui::GetCursorPosX();
    if (ImGui::Button(is_playing ? ICON_LC_SQUARE : ICON_LC_PLAY, ImVec2(30, 0))) [[unlikely]]
    {
        if (is_playing)
            g::pAudioControl->stop(output.media_id);
        else
        {
            if (g::pAudioControl->get_current() != output.media_id)
            {
                if (!load())
                    return;
            }
            g::pAudioControl->play(output.media_id);
        }
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(rect.x - (ImGui::GetCursorPosX() - asset_pos));    
    static char buffer[32] = {};
    if (ImGui::SliderFloat("##audio_timeline", &audio_pos, 0.0f, output.audio_info.duration_sec, format_timeline(buffer, output.audio_info.duration_sec, audio_pos)))
    {
        if (g::pAudioControl->get_current() != output.media_id)
        {
            if (!load())
            {
                ImGui::PopItemWidth();
                return;
            }
        }
        g::pAudioControl->seek(output.media_id, audio_pos);
    }
    ImGui::PopItemWidth();
}

void PanelQueue::show_video(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub)
{
    show_stub = true;

    bool is_playing = false;
    float video_pos = 0.0f;
    const bool first_appear = (0 == output.media_id);
    if (first_appear) [[unlikely]]
    {
        output.video_info = g::pVideoControl->get_info(output.binary_path, output.media_valid);
        output.media_info_valid = output.media_valid;
        output.video_info.thumbnail = g::pVideoControl->get_thumbnail(output.binary_path, QUEUE_IMAGE_RECT);
        output.media_id = -1;
        if (output.video_info.thumbnail.empty())
            output.media_valid = false;
        if (output.media_valid.has_value() && !output.media_valid.value())
            return;

        if (!output.video_info.thumbnail.empty())
            output.video_info.thumbnail_id = g::pTextures->load_image(output.video_info.thumbnail.c_str(), QUEUE_IMAGE_RECT, QUEUE_IMAGE_RECT, QUEUE_IMAGE_LIFETIME);
    }
    else if (output.media_id > 0)
    {
        is_playing = g::pVideoControl->get_is_playing(output.media_id);
        video_pos = g::pVideoControl->get_position(output.media_id);
    }

    const auto asset_pos = ImGui::GetCursorPosX();

    if (-1 == output.media_id || g::pVideoControl->get_current() != output.media_id)
    {
        if (!g::pTextures->get_image_alive(output.video_info.thumbnail_id))
        {
            output.video_info.thumbnail_id = g::pTextures->load_image(output.video_info.thumbnail.c_str(), QUEUE_IMAGE_RECT, QUEUE_IMAGE_RECT, QUEUE_IMAGE_LIFETIME);
            show_stub = true;

        }
        else
        {
            auto image = g::pTextures->get_image(output.video_info.thumbnail_id, output.media_valid);
            if (image)
            {
                if (output.video_info.width > 0 && output.video_info.height > 0 && output.video_info.width != output.video_info.height)
                {
                    int32_t res_x, res_y;
                    tools::img::scale(output.video_info.width, output.video_info.height, cast::to_i32(rect.x), cast::to_i32(rect.y), res_x, res_y);
                    rect.x = cast::to_float(res_x);
                    rect.y = cast::to_float(res_y);
                }

                ImGui::Image(image, rect);
                show_stub = false;
            }
        }
    }
    else [[likely]]
    {
        auto frame = g::pVideoControl->get_frame(output.media_id);
        if (frame)
        {
            if (output.video_info.width > 0 && output.video_info.height > 0 && output.video_info.width != output.video_info.height)
            {
                int32_t res_x, res_y;
                tools::img::scale(output.video_info.width, output.video_info.height, cast::to_i32(rect.x), cast::to_i32(rect.y), res_x, res_y);
                rect.x = cast::to_float(res_x);
                rect.y = cast::to_float(res_y);
            }

            ImGui::Image(frame, rect);
            show_stub = false;
        }
        else
            show_stub = true;
    }

    const auto load = [&output, &show_stub, &rect]()
    {
        output.media_id = g::pVideoControl->load(output.binary_path, QUEUE_IMAGE_RECT);
        if (0 == output.media_id)
        {
            output.media_valid = false;
            show_stub = false;
            return false;
        }
        return true;
    };

    ImGui::SetCursorPosX(asset_pos);
    if (ImGui::Button(is_playing ? ICON_LC_SQUARE : ICON_LC_PLAY, ImVec2(30, 0))) [[unlikely]]
    {
        if (is_playing)
            g::pVideoControl->stop(output.media_id);
        else
        {
            if (g::pVideoControl->get_current() != output.media_id)
            {
                if (!load())
                    return;
            }
            g::pVideoControl->play(output.media_id);
        }
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(rect.x - (ImGui::GetCursorPosX() - asset_pos));
    static char buffer[32] = {};
    if (ImGui::SliderFloat("##video_timeline", &video_pos, 0.0f, output.video_info.duration_sec, format_timeline(buffer, output.video_info.duration_sec, video_pos)))
    {
        if (g::pVideoControl->get_current() != output.media_id)
        {
            if (!load())
            {
                ImGui::PopItemWidth();
                return;
            }
        }
        g::pVideoControl->seek(output.media_id, video_pos);
    }
    ImGui::PopItemWidth();
}

void PanelQueue::on_pre_pause(PanelToolbar::Element& el) noexcept
{
    el.tooltip = g::pQueueManager->is_paused() ? "Resume" : "Pause";
    el.icon = g::pQueueManager->is_paused() ? ICON_LC_PLAY : ICON_LC_PAUSE;
    el.blinking = g::pQueueManager->is_paused();
}

void PanelQueue::on_pause(PanelToolbar::Element& /*el*/) noexcept
{
    g::pQueueManager->toggle_pause();
}

void PanelQueue::on_pre_terminate_current(PanelToolbar::Element& el) noexcept
{
    el.enabled = g::pQueueManager->get_current_group() != nullptr;
}

void PanelQueue::on_terminate_current(PanelToolbar::Element& /*el*/)
{
    g::pQueueManager->interrupt_task();
}

void PanelQueue::on_pre_remove_all(PanelToolbar::Element& el) noexcept
{
    el.enabled = g::pQueueManager->get_current_group() != nullptr || !g::pQueueManager->get_next_tasks().empty();
}

void PanelQueue::on_remove_all(PanelToolbar::Element& /*el*/) noexcept
{
    g::pQueueManager->drop_all();
}

void PanelQueue::on_pre_toggle_favorited(PanelToolbar::Element& el) noexcept
{
    el.tooltip = (m_toggled_favs ? "Hide favorited" : "Show favorited");
}

void PanelQueue::on_toggle_favorited(PanelToolbar::Element& /*el*/) noexcept
{
    m_toggled_favs = !m_toggled_favs;
}

void PanelQueue::on_pre_toggle_past(PanelToolbar::Element& el) noexcept
{
    el.enabled = !g::pQueueManager->get_past_tasks().empty();
    el.tooltip = (m_toggled_past ? "Hide completed" : "Show completed");
}

void PanelQueue::on_toggle_past(PanelToolbar::Element& /*el*/) noexcept
{
    m_toggled_past = !m_toggled_past;
}

void PanelQueue::on_pre_toggle_history(PanelToolbar::Element& el) noexcept
{
    el.enabled = !m_history_tasks.empty();
    el.tooltip = (m_toggled_history ? "Collapse all history" : "Expand all history");
}

void PanelQueue::on_toggle_history(PanelToolbar::Element& /*el*/)
{
    if (!m_toggled_history)
    {
        for (auto& task : m_history_tasks)
        {
            m_hidden.erase(task.group_id);
            m_hidden.erase(task.task_id);
        }
    }
    else
    {
        for (auto& task : m_history_tasks)
        {
            m_hidden.insert(task.group_id);
        }
    }

    m_toggled_history = !m_toggled_history;
}

void PanelQueue::on_reload_history(PanelToolbar::Element& /*el*/) noexcept
{
    m_force_reload_history = true;
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

void PanelQueue::load_history()
{
    std::string data;
    const auto history_request = fmt::format(comfyui_http::history_, g::pOptsManager->queue_history_num_load());

    const int32_t res = g::pFlow->http_get_and_wait("QPanel/History", history_request.c_str(), {}, {}, true,
        [stop_flag = m_load_history_stop]() noexcept { return stop_flag->load(std::memory_order_relaxed); }, data);

    if (res != HTTP_RESP_OK) [[unlikely]]
    {
        m_history_loading_active.store(false, std::memory_order_relaxed);
        return;
    }

    try
    {
        const auto j = o_json::parse(data);
        if (j.is_object())
        {
            for (auto& [prompt_id, jdatas] : j.items())
            {
                if (m_load_history_stop->load(std::memory_order_relaxed))
                {
                    m_history_loading_active.store(false, std::memory_order_relaxed);
                    return;
                }

                mngr::ExecTask task;
                task.group_id = base::GlobalID::random();
                task.task_id = base::GlobalID::random();
                task.loaded_from_server = true;
                task.prompt_id = prompt_id;

                if (jdatas.is_object())
                {
                    for (auto& [key, jdata] : jdatas.items())
                    {
                        if (key == "prompt")
                        {
                            if (jdata.is_array())
                            {
                                if (jdata.size() >= 1)
                                {
                                    auto& jitem = jdata[0];
                                    if (jitem.is_number_integer())
                                        task.run_no = jitem.get<uint32_t>();
                                }
                                if (jdata.size() >= 3)
                                {
                                    auto& jitem = jdata[2];
                                    if (jitem.is_object())
                                    {
                                        task.flow_json_api = jitem.dump();
                                    }
                                }
                                if (jdata.size() >= 4)
                                {
                                    auto& jitem = jdata[3];
                                    if (jitem.is_object() && jitem.contains("extra_pnginfo") && jitem["extra_pnginfo"].contains("workflow"))
                                    {
                                        auto& jwf = jitem["extra_pnginfo"]["workflow"];
                                        task.flow_json = jwf.dump();
                                    }
                                }
                            }
                        }
                        else if (key == "status")
                        {
                            if (jdata.is_object())
                            {
                                if (jdata.contains("status_str") && jdata["status_str"].is_string())
                                {
                                    if (jdata["status_str"] == "success")
                                    {
                                        task.result = TaskResult::Success;
                                        if (jdata.contains("messages") && jdata["messages"].is_array())
                                        {
                                            auto& msgs = jdata["messages"];
                                            if (msgs.is_array())
                                            {
                                                for (auto& msg : msgs)
                                                {
                                                    if (msg.is_array() && msg.size() >= 2)
                                                    {
                                                        if (msg[0].is_string() && msg[1].is_object() && msg[1].contains("timestamp") && msg[1]["timestamp"].is_number())
                                                        {
                                                            if (msg[0] == "execution_start")         task.time_start = msg[1]["timestamp"].get<uint64_t>();
                                                            else if (msg[0] == "execution_success")  task.time_end = msg[1]["timestamp"].get<uint64_t>();
                                                        }
                                                    }
                                                }
                                            }

                                            if (task.time_start != 0)
                                                task.time_start_s = tools::dt::from_time(task.time_start).as_full_date_readable(false);
                                            if (task.time_end != 0 && task.time_start != 0)
                                                task.time_ms = task.time_end - task.time_start;
                                        }
                                    }
                                    else if (jdata["status_str"] == "error")
                                    { 
                                        task.result = TaskResult::Failed;
                                        if (jdata.contains("messages") && jdata["messages"].is_array())
                                        {
                                            auto& msgs = jdata["messages"];
                                            if (msgs.is_array())
                                            {
                                                for (auto& msg : msgs)
                                                {
                                                    if (msg.is_array() && msg.size() >= 2)
                                                    {
                                                        if (msg[0].is_string() && msg[1].is_object() && msg[1].contains("timestamp") && msg[1]["timestamp"].is_number())
                                                        {
                                                            if (msg[0] == "execution_start")
                                                                task.time_start = msg[1]["timestamp"].get<uint64_t>();
                                                            else if (msg[0] == "execution_error" || msg[0] == "execution_interrupted")
                                                            {
                                                                if (msg[0] == "execution_interrupted")
                                                                {
                                                                    task.err_message = "Execution interrupted";
                                                                    append_err(task.err_full, task.err_message);
                                                                    task.result = TaskResult::Interrupted;
                                                                }

                                                                for (auto& [err_type, jerr] : msg[1].items())
                                                                {
                                                                    if (err_type == "node_type" && jerr.is_string())
                                                                        append_err(task.err_full, "Node: " + jerr.get<std::string>());
                                                                    else if (err_type == "exception_message" && jerr.is_string())
                                                                    {
                                                                        append_err(task.err_full, jerr.get<std::string>());
                                                                        task.err_message = jerr;
                                                                    }
                                                                    else if (err_type == "traceback" && jerr.is_array())
                                                                    {
                                                                        for (auto& jtrace : jerr)
                                                                        {
                                                                            if (jtrace.is_string())
                                                                                append_err(task.err_full, jtrace.get<std::string>());
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if (key == "outputs" && jdata.is_object())
                        {
                            for (auto& [o_key, jout_collection] : jdata.items())
                            {
                                if (jout_collection.is_object())
                                {
                                    for (auto& [o_type, jout] : jout_collection.items())
                                    {
                                        if ((o_type == "images" || o_type == "gifs" || o_type == "audio") && jout.is_array())
                                        {
                                            mngr::ExecTask::OutputType out_type = mngr::ExecTask::OutputType::Unknown;
                                            if (o_type == "images")
                                                out_type = mngr::ExecTask::OutputType::Image;
                                            else if (o_type == "gifs")
                                                out_type = mngr::ExecTask::OutputType::Gif;
                                            else // audio
                                                out_type = mngr::ExecTask::OutputType::Audio;

                                            for (auto& jitem : jout)
                                            {
                                                mngr::ExecTask::Output out;
                                                out.id = base::GlobalID::random();
                                                out.type = out_type;
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

                                                    if (!out.filename.empty())
                                                    {
                                                        const auto temp_filename = fs::get_temp_filepath(tools::path::path_to_string(std::filesystem::path(out.filename).extension()));
                                                        if (!temp_filename.empty())
                                                            out.binary_path = tools::path::path_to_string(temp_filename);
                                                    }
                                                }

                                                task.outputs.push_back(std::move(out));
                                            }
                                        }
                                        else if (o_type == "text" && jout.is_array())
                                        {
                                            mngr::ExecTask::Output out;
                                            out.id = base::GlobalID::random();
                                            out.type = mngr::ExecTask::OutputType::Text;
                                            for (auto& jitem : jout)
                                            {
                                                if (jitem.is_string())
                                                    out.text += jitem.get<std::string>() + "\r\n";
                                            }
                                            task.outputs.push_back(std::move(out));
                                        }
                                        else
                                        {
                                            mngr::ExecTask::Output out;
                                            out.id = base::GlobalID::random();
                                            out.type = mngr::ExecTask::OutputType::Unknown;
                                            task.outputs.push_back(std::move(out));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                std::unique_lock<std::mutex> lock(m_history_parsed_mutex);
                m_history_parsed_async.push_back(std::move(task));
            }
        }
    }
    catch (const std::exception& ex)
    {
        LogExcep("History parsing err " << ex.what());
    }

    m_history_loading_active.store(false, std::memory_order_relaxed);
}

void PanelQueue::pickup_parsed_history()
{
    if (!m_ready_pickup_history)
        return;

    if (m_history_loading_active.load(std::memory_order_relaxed))
    {
        std::unique_lock<std::mutex> lock(m_history_parsed_mutex);
        m_history_parsed_temp.splice(m_history_parsed_temp.end(), m_history_parsed_async);
    }
    else
    {
        if (!m_final_history_pickup_done)
        {
            std::unique_lock<std::mutex> lock(m_history_parsed_mutex);
            m_history_parsed_temp.splice(m_history_parsed_temp.end(), m_history_parsed_async);
            m_final_history_pickup_done = true;
        }
    }

    if (m_history_parsed_temp.empty() || !g::pFlow->get_nodes_ready())
        return;

    constexpr auto max_processing_time = 0.003; // max 3ms per frame to process tasks + sorting
    const auto start_time = ImGui::GetTime();

    for (auto it = m_history_parsed_temp.begin(); it != m_history_parsed_temp.end(); )
    {
        auto& task = *it;

        if (std::ranges::find_if(m_history_tasks,
            [&prompt_id = task.prompt_id](const mngr::ExecTask& t)
            { return t.loaded_from_db && t.prompt_id == prompt_id; }
        ) != m_history_tasks.end())
        {
            it = m_history_parsed_temp.erase(it);
            continue;
        }

        if (0 == task.time_start)
            task.time_start_s = "Time start is unknown";
        else
            task.time_start_s = tools::dt::from_time(task.time_start).as_full_date_readable(false);

        m_hidden.insert(task.group_id); // history task is hidden by default
        task.favorited = m_favorited.has_item(task.prompt_id);

        auto wfid = g::pWfsManager->import_wf_from_json(task.flow_json, task.prompt_id, {}, true);
        if (0 == wfid)
            wfid = g::pWfsManager->import_wf_from_json(task.flow_json_api, task.prompt_id, {}, true);
        if (wfid != 0)
        {
            const auto* wf = g::pWfsManager->access(wfid);
            task.flow = data::WfSerializerToBlob(wf).serialize({}).release();
            task.search_data = wf->collect_search_data();
            g::pWfsManager->erase_wf(wfid);
        }
        task.flow_json.clear(); task.flow_json.shrink_to_fit();
        task.flow_json_api.clear(); task.flow_json_api.shrink_to_fit();

        for (auto& out : task.outputs)
        {
            // here is text & unknown, load image/video/audio on demand
            if (!(mngr::ExecTask::OutputType::Audio == out.type || mngr::ExecTask::OutputType::Gif == out.type || mngr::ExecTask::OutputType::Image == out.type))
            {
                out.ready = true;
            }
        }

        g::pDbManager->history_add(task);

        m_history_tasks.push_back(std::move(task));
        it = m_history_parsed_temp.erase(it);

        if ((ImGui::GetTime() - start_time) > max_processing_time)
            break;
    }

    const auto sort_comp = [](const mngr::ExecTask& left, const mngr::ExecTask& right) noexcept
    {
        if (0 == left.time_start) return false;
        if (0 == right.time_start) return true;
        return left.time_start > right.time_start;
    };
    if (m_history_parsed_temp.empty())
        std::ranges::sort(m_history_tasks, sort_comp);
}

void PanelQueue::load_output_data(uint64_t task_id, uint64_t out_id, const std::string&& data)
{
    for (auto& task : m_history_tasks)
    {
        if (task.task_id == task_id)
        {
            for (auto& out : task.outputs)
            {
                if (out.id == out_id)
                {
                    if (data.empty())
                    { 
                        out.exists = false;
                    }
                    else
                    {
                        if (!out.binary_path.empty())
                        {
                            if (tools::path::make_file(out.binary_path, false, data) && mngr::ExecTask::OutputType::Image == out.type)
                            {
                                out.media_id = g::pTextures->load_image(out.binary_path.c_str(), QUEUE_IMAGE_RECT, QUEUE_IMAGE_RECT, QUEUE_IMAGE_LIFETIME);
                            }
                        }
                        out.exists = true;
                    }
                    out.ready = true;
                    return;
                }
            }
        }
    }
};

} // namespace ui
