
#ifdef _WIN32
#include "platform/win32/main_win32.h"
#include "platform/win32/textures_win32.h"
#elif defined __linux__
#include "platform/linux/main_linux.h"
#include "platform/linux/textures_linux.h"
#elif defined __APPLE__
#include "platform/apple/main_apple.h"
#include "platform/apple/textures_apple.h"
#endif

#include "app_instance.h"

int do_main(std::atomic_bool& request_focus)
{
    std::unique_ptr<platform::MainRoutine> main_app;
    std::unique_ptr<platform::Textures> textures;

#ifdef _WIN32
    main_app = std::make_unique<platform::MainRoutine_Win32>();
    textures = std::make_unique<platform::Textures_Win32>();
#elif defined __linux__
    main_app = std::make_unique<platform::MainRoutine_Linux>();
    textures = std::make_unique<platform::Textures_Linux>();
#elif defined __APPLE__
    main_app = std::make_unique<platform::MainRoutine_Apple>();
    textures = std::make_unique<platform::Textures_Apple>();
#endif

    if (!main_app->init())
    {
        main_app->free();
        return 1;
    }

    g::pMain = main_app.get();

    textures->set_main_routine(main_app.get());
    g::pTextures = textures.get();

    GraphN graphn;
    g::pGraphN = &graphn;
    if (!g::pGraphN->init_graphn())
        return 1;

    platform::MainRoutine::flow_callback callback = [](bool DoExit)
    {
        const bool res = g::pGraphN->flow();
        if (!res || DoExit)
        {
            if (g::pGraphN != nullptr)
                g::pGraphN->exit_graphn();
        }
        return res;
    };

    int ret = main_app->run(callback, request_focus);
    if (0 == ret)
        ret = g::pGraphN->override_exit_code();
    Log("App exit code: " << ret);

    g::pTextures = nullptr;
    g::pMain = nullptr;
    g::pGraphN = nullptr;

    main_app->free();
    return ret;
}

int main(int, char**)
{
    // ::MessageBoxW(nullptr, L"Attach", APP_CAPTION_W, MB_ICONEXCLAMATION);

    tools::dt::save_app_start_time();
    glog::LogGuard logger;    

    try
    {
        std::atomic_bool request_focus = false;
        AppInstance instance(
            [&request_focus](const std::string&)
            {
                Log("Another instance tried to launch");
                request_focus = true;
            }
        );
        if (!instance.is_primary())
            return 0;

        return do_main(request_focus);
    }
    catch (const std::exception& ex)
    {
        LogExcep(ExcepCategory::Main, "Main routine, err " << ex.what());
        throw;
    }
    catch (...)
    {
        LogExcep(ExcepCategory::Main, "Main routine unknown exception");
        throw;
    }

    return 1;
}
