
#include "ui/graph-n_flow.h"
#include "platform/main.h"
#include "core/managers/db_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/connect_manager.h"
#include "core/base/id.h"

namespace g
{
platform::MainRoutine* pMain = nullptr;
platform::Textures* pTextures = nullptr;
GraphN* pGraphN = nullptr;
ui::GraphN_Flow* pFlow = nullptr;
mngr::SettingsManager* pOptsManager = nullptr;
mngr::DbManager* pDbManager = nullptr;
mngr::NodesManager* pNodesManager = nullptr;
mngr::ConnectManager* pConnectManager = nullptr;

void save_exception(ExcepCategory category, const std::string& message, const char* file, int line)
{
    if (g::pDbManager != nullptr && category != ExcepCategory::SQLite)
        g::pDbManager->exception_save(category, message, file, line);
}
}

GraphN::GraphN()
{
    LogF;
}

GraphN::~GraphN()
{
    LogF;
}

bool GraphN::init_graphn()
{
    base::GlobalID::get_client_id();	// initialize unique client id

    m_managers.emplace_back(std::make_unique<mngr::DbManager>());
    g::pDbManager = (mngr::DbManager*)m_managers.back().get();

    m_managers.emplace_back(std::make_unique<mngr::SettingsManager>());
    g::pOptsManager = (mngr::SettingsManager*)m_managers.back().get();

    m_managers.emplace_back(std::make_unique<mngr::NodesManager>());
    g::pNodesManager = (mngr::NodesManager*)m_managers.back().get();

    m_managers.emplace_back(std::make_unique<mngr::NodesManager>());
    g::pNodesManager = (mngr::NodesManager*)m_managers.back().get();

    m_managers.emplace_back(std::make_unique<mngr::ConnectManager>());
    g::pConnectManager = (mngr::ConnectManager*)m_managers.back().get();

    bool success = true;
    std::for_each(m_managers.begin(), m_managers.end(), [&success](std::unique_ptr<mngr::Manager>& manager)
    {
        if (success && !manager->init())
            success = false;
    });
    if (!success)
    {
        Log("Failed to initialize managers");
        return false;
    }
    Log("Managers ready");

    g::pMain->set_vsync_enabled(g::pOptsManager->app_vsync_load());

    m_flow_step = Flow::Init;
    return true;
}

void GraphN::exit_graphn()
{
    LogF;

    if (m_graphn_flow)
        m_graphn_flow->exit();
    m_graphn_flow.reset();
    g::pFlow = nullptr;

    while (m_block_db_on_exit)
        ;

    std::for_each(m_managers.begin(), m_managers.end(), [](std::unique_ptr<mngr::Manager>& manager)
    {
        manager->stop_all_threads();
        manager->free();
    });

    g::pNodesManager = nullptr;
    g::pOptsManager = nullptr;
    g::pDbManager = nullptr;
    g::pConnectManager = nullptr;

    LogF;
}

int GraphN::override_exit_code()
{
    return m_exit_code;
}

void GraphN::handle_wnd_minimized()
{}

void GraphN::handle_wnd_restored()
{}

void GraphN::handle_wnd_on_close()
{
    LogF;
    m_flow_step = Flow::PrepareToExit;
}

void GraphN::handle_1sec_timer()
{}

bool GraphN::flow()
{
    bool result = false;
    try
    {
        switch (m_flow_step)
        {
        case Flow::Init:
            result = init();
            break;
        case Flow::GraphN:
            result = run_graphn();
            break;
        case Flow::PrepareToExit:
            if (m_graphn_flow)
                m_graphn_flow->exit();
            m_flow_step = Flow::DoExit;
            result = true;
            break;
        case Flow::DoExit:
            result = false;
            break;
        default:
            LogAndThrowInternalError(ExcepCategory::SwitchEnum, "Unhandler flow step " << cast::to_u64(m_flow_step));
        }
    }
    catch (const std::exception& ex)
    {
        result = false;
        LogExcep(ExcepCategory::Flow, ex.what());
        throw;
    }

    return result;
}

bool GraphN::init()
{
    LogF;

    _tzset();

    m_flow_step = Flow::GraphN;
    return true;
}

bool GraphN::run_graphn()
{
    if (nullptr == m_graphn_flow)
    {
        Log("Creating Graph-N flow");
        m_graphn_flow = std::make_unique<ui::GraphN_Flow>();
        g::pFlow = m_graphn_flow.get();
        if (!g::pFlow->init())
        {
            Log("Graph-N flow initialization failed");
            return false;
        }
    }

    const bool res = m_graphn_flow->run();
    if (!res)
    {
        Log("GraphN flow exited, next flow step: exit");
        m_flow_step = Flow::DoExit;
    }
    return res;
}


