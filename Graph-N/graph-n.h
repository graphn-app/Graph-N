
#pragma once

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
        init,
        graph_n,
        prepare_to_exit,
        do_exit
    };

    bool init_graphn();
    void exit_graphn();

    bool flow();
    int override_exit_code();

public:
    // main wnd handlers
    void handle_wnd_minimized();
    void handle_wnd_restored();
    void handle_wnd_on_close();
    void handle_1sec_timer();

private:
    // flow
    bool init();
    bool run_graphn();

private:
    Flow m_flow_step = Flow::_none;
    bool m_comfyui_connected = false;

    std::unique_ptr<ui::GraphN_Flow> m_graphn_flow;

    int m_exit_code = 0;

    std::vector<std::unique_ptr<mngr::Manager>> m_managers;
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
}
