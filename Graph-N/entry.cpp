#ifdef _WIN32
#include "main/main_win32.h"
#elif defined __linux__
#include "main/main_linux.h"
#elif defined __APPLE__
#include "main/main_apple.h"
#endif

#include "app_instance.h"

int32_t do_main(std::atomic_bool& request_focus)
{
    std::unique_ptr<global::MainRoutine> main_app;

#ifdef _WIN32
    main_app = std::make_unique<platform::MainRoutine_Win32>();
#elif defined __linux__
    main_app = std::make_unique<platform::MainRoutine_Linux>();
#elif defined __APPLE__
    main_app = std::make_unique<platform::MainRoutine_Apple>();
#endif

    if (!main_app->init())
    {
        main_app->free();
        return 1;
    }

    g::pMain = main_app.get();

    int32_t ret = 0;
    {
        GraphN graphn;
        g::pGraphN = &graphn;
        if (!g::pGraphN->init_graphn())
            return 1;

        const global::MainRoutine::cleanup_callback cleanup_callback = []()
        {
            g::pGraphN->cleanup_frame();
        };

        const global::MainRoutine::flow_callback flow_callback = [](bool DoExit)
        {
            const bool res = g::pGraphN->flow();
            if (!res || DoExit)
            {
                if (g::pGraphN != nullptr)
                    g::pGraphN->exit_graphn();
            }
            return res;
        };

        ret = main_app->run(flow_callback, cleanup_callback, request_focus);
        if (0 == ret)
            ret = g::pGraphN->override_exit_code();
        LogS("App exit code: " << ret);
    }
    
    g::pGraphN = nullptr;
    g::pMain = nullptr;
    main_app->free();
    return ret;
}

int32_t main(int32_t, char**)
{
    base::GlobalID::client_id();
    tools::dt::save_app_start_time();
    glog::LogGuard logger;

    try
    {
        std::atomic_bool request_focus = false;
        AppInstance instance(
            [&request_focus](const std::string&)
            {
                LogS("Another instance tried to launch");
                request_focus.store(true, std::memory_order_relaxed);
            }
        );
        if (!instance.is_primary())
            return 0;

        const auto ret = do_main(request_focus);
        return ret;
    }
    catch (const std::exception& ex)
    {
        LogExcepS("Main routine ex " << ex.what());
        throw;
    }
    catch (...)
    {
        LogExcepS("Main routine unknown exception");
        throw;
    }

    return 1;
}
