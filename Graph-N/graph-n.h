#pragma once
#include "core/listeners.h"

namespace global
{
class MainRoutine;
class Textures;
class AudioControl;
class VideoControl;
}

namespace mngr
{
class Manager;
class SettingsManager;
class DbManager;
class NodesManager;
class ConnectManager;
class WfsManager;
class ModelsManager;
class ComfyUIManager;
class HistoryManager;
class QueueManager;
class HotkeysManager;
class HostManager;
}

namespace ui
{
class GraphN_Flow;
}

class GraphN final
{
public:
    GraphN() noexcept;
    ~GraphN() noexcept;

    NO_COPY_MOVE(GraphN);

    enum class Flow : uint8_t
    {
        _none,
        Init,
        StartServer,
        Connect,
        GraphN,
        PrepareToExit,
        DoExit
    };

    bool init_graphn();
    void exit_graphn();

    bool flow();
    void cleanup_frame();
    int32_t override_exit_code() noexcept;

    void subscribe_timers(listener::Timer* listener);
    void unsubscribe_timers(listener::Timer* listener) noexcept;

    void subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step step, listener::ComfyUiObjectValidityListener* listener);
    void unsubscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener* listener) noexcept;

    void invalidate_comfyui_objects();
    void resolve_comfyui_objects();

public:
    // main wnd handlers
    void handle_wnd_minimized() noexcept {}
    void handle_wnd_restored() noexcept {}
    void handle_wnd_on_close() noexcept;
    void handle_1sec_timer(int64_t seconds_elapsed);
    void handle_new_frame(int64_t frames_elapsed);

private:
    // flow
    bool init();
    bool start_server();
    bool connect() noexcept;
    bool run_graphn();

private:
    Flow m_flow_step = Flow::_none;
    bool m_question_mode = false;
    bool m_question_replied = false;

    int32_t m_exit_code = 0;

    std::unique_ptr<ui::GraphN_Flow> m_graphn_flow;
    std::vector<std::unique_ptr<mngr::Manager>> m_managers;
    std::unique_ptr<global::Textures> m_textures;
    std::unique_ptr<global::AudioControl> m_audio;
    std::unique_ptr<global::VideoControl> m_video;

    std::list<listener::Timer*> m_timer_listeners;
    std::map<listener::ComfyUiObjectValidityListener::Step, std::list<listener::ComfyUiObjectValidityListener*>> m_comfyui_validity_listeners;
};


namespace g
{
extern global::MainRoutine* pMain;
extern global::Textures* pTextures;
extern global::AudioControl* pAudioControl;
extern global::VideoControl* pVideoControl;

extern GraphN* pGraphN;
extern ui::GraphN_Flow* pFlow;

extern mngr::SettingsManager* pOptsManager;
extern mngr::DbManager* pDbManager;
extern mngr::NodesManager* pNodesManager;
extern mngr::ConnectManager* pConnectManager;
extern mngr::WfsManager* pWfsManager;
extern mngr::ModelsManager* pModelsManager;
extern mngr::ComfyUIManager* pComfyUIManager;
extern mngr::HistoryManager* pHistoryManager;
extern mngr::QueueManager* pQueueManager;
extern mngr::HotkeysManager* pHotkeysManager;
extern mngr::HostManager* pHostManager;

extern std::stop_source app_global_stop_all_threads;
extern uint32_t block_hotkeys_mask;

void save_exception(std::string message, const char* file, int32_t line) noexcept;
}
