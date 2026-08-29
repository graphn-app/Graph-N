#include "graph-n.h"
#include "ui/graph-n_flow.h"
#include "ui/helpers/textures.h"
#include "core/managers/db_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/connect_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/models_manager.h"
#include "core/managers/history_manager.h"
#include "core/managers/queue_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "core/managers/host_manager.h"
#if defined _WIN32
#include "core/managers/comfyui_manager_win32.h"
#include "core/managers/host_manager_win32.h"
#elif defined __linux__
#include "core/managers/comfyui_manager_linux.h"
#include "core/managers/host_manager_linux.h"
#elif defined __APPLE__
#include "core/managers/comfyui_manager_apple.h"
#include "core/managers/host_manager_apple.h"
#endif
#include "core/base/id.h"
#include "main.h"

namespace g
{
global::MainRoutine* pMain = nullptr;
global::Textures* pTextures = nullptr;
global::AudioControl* pAudioControl = nullptr;
global::VideoControl* pVideoControl = nullptr;

GraphN* pGraphN = nullptr;
ui::GraphN_Flow* pFlow = nullptr;

mngr::SettingsManager* pOptsManager = nullptr;
mngr::DbManager* pDbManager = nullptr;
mngr::NodesManager* pNodesManager = nullptr;
mngr::ConnectManager* pConnectManager = nullptr;
mngr::WfsManager* pWfsManager = nullptr;
mngr::ModelsManager* pModelsManager = nullptr;
mngr::ComfyUIManager* pComfyUIManager = nullptr;
mngr::HistoryManager* pHistoryManager = nullptr;
mngr::QueueManager* pQueueManager = nullptr;
mngr::HotkeysManager* pHotkeysManager = nullptr;
mngr::HostManager* pHostManager = nullptr;

std::stop_source app_global_stop_all_threads;
int32_t block_hotkeys_counter = 0;

void save_exception(std::string message, const char* file, int32_t line) noexcept
{
    if (g::pDbManager != nullptr)
        g::pDbManager->exception_save(std::move(message), file, line);
}
}

GraphN::GraphN() noexcept
{
    Log("");
}

GraphN::~GraphN() noexcept
{
    Log("");
}

bool GraphN::init_graphn()
{
    LogIn;

    m_managers.push_back(std::make_unique<mngr::DbManager>());
    g::pDbManager = cast::ptrs<mngr::DbManager>(m_managers.back().get());

    m_managers.push_back(std::make_unique<mngr::SettingsManager>());
    g::pOptsManager = cast::ptrs<mngr::SettingsManager>(m_managers.back().get());

    m_managers.push_back(std::make_unique<mngr::NodesManager>());
    g::pNodesManager = cast::ptrs<mngr::NodesManager>(m_managers.back().get());

    m_managers.push_back(std::make_unique<mngr::ModelsManager>());
    g::pModelsManager = cast::ptrs<mngr::ModelsManager>(m_managers.back().get());

    m_managers.push_back(std::make_unique<mngr::ConnectManager>());
    g::pConnectManager = cast::ptrs<mngr::ConnectManager>(m_managers.back().get());

    m_managers.push_back(std::make_unique<mngr::HistoryManager>());
    g::pHistoryManager = cast::ptrs<mngr::HistoryManager>(m_managers.back().get());

    m_managers.push_back(std::make_unique<mngr::WfsManager>());
    g::pWfsManager = cast::ptrs<mngr::WfsManager>(m_managers.back().get());

#if defined _WIN32
    m_managers.push_back(std::make_unique<mngr::ComfyUIManager_Win32>());
#elif defined __linux__
    m_managers.push_back(std::make_unique<mngr::ComfyUIManager_Linux>());
#elif defined __APPLE__
    m_managers.push_back(std::make_unique<mngr::ComfyUIManager_Apple>());
#endif
    g::pComfyUIManager = cast::ptrs<mngr::ComfyUIManager>(m_managers.back().get());

    m_managers.push_back(std::make_unique<mngr::QueueManager>());
    g::pQueueManager = cast::ptrs<mngr::QueueManager>(m_managers.back().get());

    m_managers.push_back(std::make_unique<mngr::HotkeysManager>());
    g::pHotkeysManager = cast::ptrs<mngr::HotkeysManager>(m_managers.back().get());

#if defined _WIN32
    m_managers.push_back(std::make_unique<mngr::HostManager_Win32>());
#elif defined __linux__
    m_managers.push_back(std::make_unique<mngr::HostManager_Linux>());
#elif defined __APPLE__
    m_managers.push_back(std::make_unique<mngr::HostManager_Apple>());
#endif
    g::pHostManager = cast::ptrs<mngr::HostManager>(m_managers.back().get());

    bool success = true;
    std::for_each(m_managers.begin(), m_managers.end(), [&success](std::unique_ptr<mngr::Manager>& manager)
    {
        if (success && !manager->init())
            success = false;
    });
    LogCriticalIf(success, "Failed to initialize managers");

    Log("Managers ready");

    g::pMain->set_vsync_enabled(g::pOptsManager->app_vsync_load());

    m_flow_step = Flow::Init;
    Log("Next step: init");
    LogOut;
    return true;
}

void GraphN::exit_graphn()
{
    LogIn;

    g::app_global_stop_all_threads.request_stop();
    g::pTextures->cleanup_all();

    std::for_each(m_managers.rbegin(), m_managers.rend(), [](std::unique_ptr<mngr::Manager>& manager)
    {
        manager->stop_all_threads();
        manager->free();
    });

    if (m_graphn_flow)
        m_graphn_flow->exit();
    m_graphn_flow.reset();

    g::pFlow = nullptr;
    g::pQueueManager = nullptr;
    g::pNodesManager = nullptr;
    g::pOptsManager = nullptr;
    g::pDbManager = nullptr;
    g::pConnectManager = nullptr;

    auto temp = fs::get_temp_directory_path();
    tools::path::remove(temp);

    LogOut;
}

int32_t GraphN::override_exit_code() noexcept
{
    LogV(m_exit_code);
    return m_exit_code;
}

void GraphN::handle_wnd_on_close() noexcept
{
    m_flow_step = Flow::PrepareToExit;
    Log("Next step: PrepareToExit");
}

void GraphN::subscribe_timers(listener::Timer* listener)
{
    m_timer_listeners.push_back(listener);
}

void GraphN::unsubscribe_timers(listener::Timer* listener) noexcept
{
    try {
        m_timer_listeners.remove(listener);
    } catch(...) {}
}

void GraphN::handle_1sec_timer(int64_t seconds_elapsed)
{
    for (auto* listener : m_timer_listeners)
    {
        listener->on_1s_lazy_timer(seconds_elapsed);
    }
}

void GraphN::subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step step, listener::ComfyUiObjectValidityListener* listener)
{
    m_comfyui_validity_listeners[step].push_back(listener);
}

void GraphN::unsubscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener* listener) noexcept
{
    try
    {
        for (auto& listeners : m_comfyui_validity_listeners)
        {
            listeners.second.remove(listener);
        }
    }
    catch (...) {}
}

void GraphN::invalidate_comfyui_objects()
{
    Log("Notify listeners");

    auto& listeners = m_comfyui_validity_listeners[listener::ComfyUiObjectValidityListener::Step::Invalidate];
    for (auto listener : listeners)
    {
        listener->invalidate_comfyui_objects();
    }
}

void GraphN::resolve_comfyui_objects()
{
    Log("Notify listeners");

    const auto notify = [this](listener::ComfyUiObjectValidityListener::Step step)
    {
        auto& listeners = m_comfyui_validity_listeners[step];
        for (auto listener : listeners)
        {
            listener->resolve_comfyui_objects(step);
        }
    };

    notify(listener::ComfyUiObjectValidityListener::Step::ResolveNodes);
    notify(listener::ComfyUiObjectValidityListener::Step::ResolvePins);
    notify(listener::ComfyUiObjectValidityListener::Step::ResolveWfs);
    notify(listener::ComfyUiObjectValidityListener::Step::ResolvePanels);
}

void GraphN::handle_new_frame(int64_t frames_elapsed)
{
    for (auto* listener : m_timer_listeners)
    {
        listener->on_frame(frames_elapsed);
    }
}

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
        case Flow::StartServer:
            result = start_server();
            break;
        case Flow::Connect:
            result = connect();
            break;
        case Flow::GraphN: [[likely]]
            result = run_graphn();
            break;
        case Flow::PrepareToExit:
            if (m_graphn_flow)
                m_graphn_flow->exit();
            m_flow_step = Flow::DoExit;
            Log("Next step: DoExit");
            result = true;
            break;
        case Flow::DoExit:
            result = false;
            break;
        default:
            LogRError("Unhandler flow step " << cast::to_u64(m_flow_step), false);
        }

        handle_new_frame(ImGui::GetFrameCount());
    }
    catch (const std::exception& ex)
    {
        result = false;
        LogCritical("Main flow ex: " << ex.what());
    }

    return result;
}

bool GraphN::init()
{
    _tzset();

    Log("Creating Graph-N flow");
    m_graphn_flow = std::make_unique<ui::GraphN_Flow>();
    g::pFlow = m_graphn_flow.get();
    LogCriticalIf(g::pFlow->init(), "Graph-N flow initialization failed");

    g::pTextures->init_ui_ready();

    m_flow_step = Flow::StartServer;
    Log("Next step: StartServer");
    return true;
}

bool GraphN::start_server()
{
    const auto handler = [this](ModalDlgCtrl res)
    {
        if (ModalDlgCtrl::BtnYes == res)
        {
            LogS("Starting " APP_COMFYUI " server");
            g::pComfyUIManager->run(false);
        }
        m_question_mode = false;
        m_question_replied = true;
    };

    if (m_question_mode)
    {
        g::pFlow->modal_show();
        return true;
    }
    if (m_question_replied)
    {
        m_flow_step = Flow::Connect;
        Log("Next step: Connect");
        m_question_replied = false;
        return true;
    }

    if (!g::pOptsManager->comfyui_start_server_load())
    {
        m_flow_step = Flow::Connect;
        Log("Next step: Connect");
        return true;
    }

    const auto running_proc_id = g::pComfyUIManager->check_existing_process();
    if (running_proc_id != 0)
    {
        Log("Existing " APP_COMFYUI " found. Ask user what to do");
        g::pFlow->modal_begin("The " APP_COMFYUI " is already running",
            fmt::format(FMT_STRING(APP_CAPTION " detected a running " APP_COMFYUI "\nprocess (PID: {}).\n"
            "This might be another " APP_COMFYUI " instance running, or a zombie process.\n"
            "Do you want to launch a new server instance anyway?"), running_proc_id),
            ModalDlgCtrl::BtnYes | ModalDlgCtrl::BtnNo | ModalDlgCtrl::IcoQues, ModalDlgCtrl::BtnNo, handler);
        m_question_mode = true;
        m_question_replied = false;
        return true;
    }

    Log("Starting " APP_COMFYUI " server");
    g::pComfyUIManager->run(false);
    m_flow_step = Flow::Connect;
    Log("Next step: Connect");
    return true;
}

bool GraphN::connect() noexcept
{
    //g::pFlow->reconnect();
    m_flow_step = Flow::GraphN;
    Log("Next step: GraphN");
    return true;
}

bool GraphN::run_graphn()
{
    const bool res = m_graphn_flow->run();
    if (!res) [[unlikely]]
    {
        m_flow_step = Flow::DoExit;
        Log("Next step: DoExit");
    }
    return res;
}
