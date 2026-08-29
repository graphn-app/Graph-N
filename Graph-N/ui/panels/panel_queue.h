#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelQueue final
    : public PanelBase<PanelCommonData>
    , public PanelToolbar
    , public PanelEmptyWithDescr
    , public IsAlive
{
public:
    PanelQueue();
    ~PanelQueue() noexcept override;

    void init() override;
    void stop_all_threads() noexcept override;

private:
    DlgCode show(bool& focused) override;
    void show_current_task();
    void show_external_task();
    void show_next_tasks();
    void show_favs_tasks();
    void show_past_tasks();
    void show_history_tasks();
    void show_past_task(uint64_t group_id, mngr::ExecTask* task, const std::string& wf_name = "");
    int32_t show_tasks_num(int32_t curr) noexcept;
    void show_image(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub);
    void show_audio(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub);
    void show_video(mngr::ExecTask::Output& output, ImVec2 rect, bool& show_stub);
    void show_asset_info(const mngr::ExecTask::Output& output, size_t out_idx);
    void create_toolbar();

    void init_history();
    void load_history();
    void pickup_parsed_history();
    void load_output_data(uint64_t task_id, uint64_t out_id, const std::string&& data);

    void on_pre_pause(PanelToolbar::Element& el) noexcept;
    void on_pause(PanelToolbar::Element& el) noexcept;
    void on_pre_terminate_current(PanelToolbar::Element& el) noexcept;
    void on_terminate_current(PanelToolbar::Element& el);
    void on_pre_remove_all(PanelToolbar::Element& el) noexcept;
    void on_remove_all(PanelToolbar::Element& el) noexcept;
    void on_pre_toggle_favorited(PanelToolbar::Element& el) noexcept;
    void on_toggle_favorited(PanelToolbar::Element& el) noexcept;
    void on_pre_toggle_past(PanelToolbar::Element& el) noexcept;
    void on_toggle_past(PanelToolbar::Element& el) noexcept;
    void on_pre_toggle_history(PanelToolbar::Element& el) noexcept;
    void on_toggle_history(PanelToolbar::Element& el);
    void on_reload_history(PanelToolbar::Element& el) noexcept;

private:
    float m_curr_pos_x = 0.0f;
    float m_curr_pos2_x = 0.0f;

    bool m_visible_curr = false;
    bool m_visible_pend = false;
    bool m_visible_favs = false;
    bool m_visible_past = false;

    bool m_toggled_favs = true;
    bool m_toggled_past = true;
    bool m_toggled_history = false;

    std::set<std::string> m_favorited;
    std::set<uint64_t> m_hidden;

    std::vector<mngr::ExecTask> m_history_tasks;
    std::list<mngr::ExecTask> m_history_parsed_async;
    std::list<mngr::ExecTask> m_history_parsed_temp;
    std::mutex m_history_parsed_mutex;

    std::atomic_bool m_history_loading_active = false;
    std::shared_ptr<std::atomic_bool> m_load_history_stop;
    std::atomic_bool m_do_reload_history = true;
    bool m_ready_pickup_history = false;
    bool m_final_history_pickup_done = false;
    bool m_force_reload_history = false;

    bool m_fix_panel_scroll = false;

    const size_t m_completed_display_limit = 15;
    size_t m_current_completed_displays = 15;
    const size_t m_history_display_limit = 50;
    size_t m_current_history_displays = 50;

    ImGuiEx::Toast m_toast;
};


} // namespace ui
