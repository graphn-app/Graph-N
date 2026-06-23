
#ifdef _WIN32
#include "main/main_win32.h"
#include "ui/helpers/textures_win32.h"
#include "ui/helpers/video_control_win32.h"
#elif defined __linux__
#include "main/main_linux.h"
#include "ui/helpers/textures_linux.h"
#include "ui/helpers/video_control_linux.h"
#elif defined __APPLE__
#include "main/main_apple.h"
#include "ui/helpers/textures_apple.h"
#include "ui/helpers/video_control_apple.h"
#endif

#include "app_instance.h"
#include "ui/helpers/audio_control.h"

int32_t do_main(std::atomic_bool& request_focus)
{
    std::unique_ptr<global::MainRoutine> main_app;
    std::unique_ptr<global::Textures> textures;
    std::unique_ptr<global::AudioControl> audio;
    std::unique_ptr<global::VideoControl> video;

#ifdef _WIN32
    main_app = std::make_unique<platform::MainRoutine_Win32>();
    textures = std::make_unique<platform::Textures_Win32>();
    video = std::make_unique<platform::VideoControl_Win32>();
#elif defined __linux__
    main_app = std::make_unique<platform::MainRoutine_Linux>();
    textures = std::make_unique<platform::Textures_Linux>();
    video = std::make_unique<platform::VideoControl_Linux>();
#elif defined __APPLE__
    main_app = std::make_unique<platform::MainRoutine_Apple>();
    textures = std::make_unique<platform::Textures_Apple>();
    video = std::make_unique<platform::VideoControl_Apple>();
#endif

    if (!main_app->init())
    {
        main_app->free();
        return 1;
    }

    g::pMain = main_app.get();
    g::pTextures = textures.get();

    audio = std::make_unique<global::AudioControl>();
    audio->init();
    g::pAudioControl = audio.get();

    video->init();
    g::pVideoControl = video.get();

    GraphN graphn;
    g::pGraphN = &graphn;
    if (!g::pGraphN->init_graphn())
        return 1;

    global::MainRoutine::flow_callback callback = [](bool DoExit)
    {
        const bool res = g::pGraphN->flow();
        if (!res || DoExit)
        {
            if (g::pGraphN != nullptr)
                g::pGraphN->exit_graphn();
        }
        return res;
    };

    int32_t ret = main_app->run(callback, request_focus);
    if (0 == ret)
        ret = g::pGraphN->override_exit_code();
    LogS("App exit code: " << ret);

    g::pAudioControl->cleanup_all();
    g::pAudioControl = nullptr;
    g::pVideoControl->cleanup();
    g::pVideoControl = nullptr;
    g::pTextures->cleanup_all();
    g::pTextures = nullptr;
    g::pMain = nullptr;
    g::pGraphN = nullptr;

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
                request_focus = true;
            }
        );
        if (!instance.is_primary())
            return 0;

        const auto ret = do_main(request_focus);
        glog::Logger::cleanup();
        return ret;
    }
    catch (const std::exception& ex)
    {
        LogExcepS(ExcepCategory::Main, "Main routine, err " << ex.what());
        throw;
    }
    catch (...)
    {
        LogExcepS(ExcepCategory::Main, "Main routine unknown exception");
        throw;
    }

    return 1;
}
