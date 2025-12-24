#include <gtest/gtest.h>

class ImGuiEnvironment : public ::testing::Environment {
public:
    void SetUp() override 
	{
        ImGui::CreateContext();
        
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1920, 1080);
        io.DeltaTime = 1.0f / 60.0f;
    }

    void TearDown() override 
	{
        ImGui::DestroyContext();
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