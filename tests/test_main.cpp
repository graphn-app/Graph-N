/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include <gtest/gtest.h>

#include "core/managers/db_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/connect_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/models_manager.h"
#include "core/managers/history_manager.h"
#if defined _WIN32
#include "core/managers/comfyui_manager_win32.h"
#include "main/main_win32.h"
#include "ui/helpers/textures_win32.h"
#elif defined __linux__
#include "core/managers/comfyui_manager_linux.h"
#include "main/main_linux.h"
#include "ui/helpers/textures_linux.h"
#elif defined __APPLE__
#include "core/managers/comfyui_manager_apple.h"
#include "main/main_apple.h"
#include "ui/helpers/textures_apple.h"
#endif
#include "ui/graph-n_flow.h"

class ImGuiEnvironment : public ::testing::Environment
{
    std::unique_ptr<global::MainRoutine> main_app;
    std::unique_ptr<global::Textures> textures;
    GraphN graphn;

public:
    void SetUp() override 
	{
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(1920.0f, 1080.0f);
        io.DeltaTime = 1.0f / 60.0f;

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

        main_app->init();
        g::pMain = main_app.get();
        g::pTextures = textures.get();

        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        ImGui::NewFrame();
        ImGui::Begin("TestWindow");

        g::pGraphN = &graphn;
        g::pGraphN->init_graphn();
        g::pGraphN->flow();
    }

    void TearDown() override 
	{
        ImGui::End();
        ImGui::Render();
        ImGui::DestroyContext();

        g::pGraphN->exit_graphn();
    }
};

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new ImGuiEnvironment);
    return RUN_ALL_TESTS();
}

TEST(SanityCheck, TrueIsTrue) 
{
    EXPECT_EQ(true, true);
}
