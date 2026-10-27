/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/panels/panel_base.h"
#include "core/listeners.h"

namespace ui
{

class PanelQueue final
    : public PanelBase<PanelCommonData>
    , public PanelToolbar
    , public PanelEmptyWithDescr
    , public IsAlive
    , public listener::HotkeysChangedHandler
{
public:
    PanelQueue();
    ~PanelQueue() noexcept override;

    void init() override;
    void stop_and_cleanup() noexcept override;

    bool get_favorited_visible() const noexcept { return m_view_favs; }
    bool get_completed_visible() const noexcept { return m_view_completed; }
    bool get_history_visible() const noexcept { return m_view_history; }

    void toggle_favorited_visible() noexcept { m_view_favs = !m_view_favs; }
    void toggle_completed_visible() noexcept { m_view_completed = !m_view_completed; }
    void toggle_history_visible() noexcept { m_view_history = !m_view_history; }

    void force_reload_history() noexcept { m_force_reload_history = true; }

private:
    DlgCode show(bool& focused) override;
    void show_current_task();
    void show_external_task();
    void show_next_tasks();
    void show_favs_tasks();
    void show_completed_tasks();
    void show_history_tasks();
    void show_completed_task(uint64_t group_id, mngr::ExecTask* task, const std::string& wf_name = "");
    int32_t show_tasks_num(int32_t curr) noexcept;
    void show_image(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub);
    void show_audio(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub);
    void show_video(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub);
    void show_asset_info(const mngr::ExecTask::Output& output, size_t out_idx);
    void create_toolbar();
    void handle_hotkeys_changed() override; // HotkeysChangedHandler

    void init_history();
    void load_history();
    void pickup_parsed_history();
    void load_output_data(uint64_t task_id, uint64_t out_id, const std::string&& data);

private:
    float m_curr_pos_x = 0.0f;
    float m_curr_pos2_x = 0.0f;

    bool m_visible_curr = false;
    bool m_visible_pend = false;
    bool m_visible_favs = false;
    bool m_visible_completed = false;

    bool m_view_favs = true;
    bool m_view_completed = true;
    bool m_view_history = true;

    bool m_collapse_all_tasks = false;
    bool m_collapse_all_outputs = false;

    mngr::StarredList m_favorited;
    std::set<uint64_t> m_hidden;

    std::vector<mngr::ExecTask> m_history_tasks;
    std::list<mngr::ExecTask> m_history_parsed_async;
    std::list<mngr::ExecTask> m_history_parsed_temp;
    std::mutex m_history_parsed_mutex;

    std::atomic_bool m_history_loading_active = false;
    std::shared_ptr<std::atomic_bool> m_load_history_stop;
    bool m_do_reload_history = true;
    bool m_ready_pickup_history = false;
    bool m_final_history_pickup_done = false;
    bool m_force_reload_history = false;

    bool m_fix_panel_scroll = false;

    const size_t m_completed_display_limit = 15;
    size_t m_current_completed_displays = 15;
    const size_t m_history_display_limit = 50;
    size_t m_current_history_displays = 50;
};


} // namespace ui
