
#pragma once
#include "core/listeners.h"

namespace platform
{
class MainRoutine;
class Textures;
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
}

namespace ui
{
class GraphN_Flow;
}

class GraphN final
{
public:
    GraphN();
    ~GraphN();

    NO_COPY_MOVE(GraphN);

    enum class Flow
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
    int32_t override_exit_code();

    void subscribe_timers(listener::Timer* listener);
    void unsubscribe_timers(listener::Timer* listener);

    void subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step step, listener::ComfyUiObjectValidityListener* listener);
    void unsubscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener* listener);

    void invalidate_comfyui_objects();
    void resolve_comfyui_objects();

public:
    // main wnd handlers
    void handle_wnd_minimized() {}
    void handle_wnd_restored() {}
    void handle_wnd_on_close();
    void handle_1sec_timer(int64_t seconds_elapsed);
    void handle_new_frame(int64_t frames_elapsed);

private:
    // flow
    bool init();
    bool start_server();
    bool connect();
    bool run_graphn();

private:
    Flow m_flow_step = Flow::_none;
    bool m_question_mode = false;
    bool m_question_replied = false;

    int32_t m_exit_code = 0;

    std::unique_ptr<ui::GraphN_Flow> m_graphn_flow;
    std::vector<std::unique_ptr<mngr::Manager>> m_managers;

    std::list<listener::Timer*> m_timer_listeners;
    std::map<listener::ComfyUiObjectValidityListener::Step, std::list<listener::ComfyUiObjectValidityListener*>> m_comfyui_validity_listeners;
};


namespace g
{
extern platform::MainRoutine* pMain;
extern platform::Textures* pTextures;

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

void save_exception(ExcepCategory category, std::string message, const char* file, int32_t line);
}
