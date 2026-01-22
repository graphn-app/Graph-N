
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
class WfsManager;
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
        GraphN,
        PrepareToExit,
        DoExit
    };

    bool init_graphn();
    void exit_graphn();

    bool flow();
    int override_exit_code();

    inline void block_db_on_exit() { m_block_db_on_exit = true; }
    inline void unblock_db_on_exit() { m_block_db_on_exit = false; }

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
    int m_exit_code = 0;

    std::unique_ptr<ui::GraphN_Flow> m_graphn_flow;

    std::atomic_bool m_block_db_on_exit = false;
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
extern mngr::WfsManager* pWfsManager;

void save_exception(ExcepCategory category, std::string message, const char* file, int line);
}
