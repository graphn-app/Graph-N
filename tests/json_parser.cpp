#include <gtest/gtest.h>
#include "core/managers/nodes_manager.h"

class ParserTest : public ::testing::Test
{
private:
    std::unique_ptr<mngr::NodesManager> manager;

public:
    void SetUp() override
    {
        manager = std::make_unique<mngr::NodesManager>();
        g::pNodesManager = manager.get();
    }

    void TearDown() override
    {
    }

    bool parse(const std::string& json)
    {
        mngr::NodesParser parser;
        parser.parse(json);

        size_t parsed_item = 0, parse_total = 0;
        do
        {
            parser.check_status(parsed_item, parse_total);
        } while (!parser.is_parse_finished());

        return !parser.is_parse_failed();
    }
};


TEST_F(ParserTest, HandlesEmptyJson)
{
    bool parsed = parse("{}");
    ASSERT_TRUE(parsed);

    EXPECT_EQ(g::pNodesManager->get_nodes_sorted().size(), 4);
}


TEST_F(ParserTest, HandlesCrazyInput)
{
    const std::string test_json =
        R"({
        "CrazyNode_v1": {
            "name": "CrazyNode",
            "display_name": "🤪 Node With Emojis",
            "category": "Testing/Dangerous",
            "input": {
                "required": {
                    "normal_int": ["INT", {"default": 5, "min": 0, "max": 10}],
                    "broken_float": ["FLOAT", {"default": "NOT_A_NUMBER", "step": 0.1}],
                    "weird_combo": [["one", 2, true, null]], 
                    "missing_config": ["INT"],
                    "any_pin": ["*", {}]
                },
                "optional": {
                    "mixed_types": ["STRING", {"multiline": true, "default": 12345}]
                }
            },
            "output": ["IMAGE", "*", "LATENT"],
            "output_name": ["OutImg", "AnyOut", "LatentOut"],
            "output_is_list": [false, true, false]
        }
    })";

    glog::LogGuard logger;
    bool parsed = parse(test_json);
    ASSERT_TRUE(parsed);

    auto& nodes = g::pNodesManager->get_nodes_sorted();

    EXPECT_EQ(nodes.size(), 5);
    EXPECT_EQ(nodes.count("CrazyNode"), 1);

    auto it = nodes.find("CrazyNode");
    ASSERT_FALSE(it == nodes.end());

    auto node = it->second;
    EXPECT_EQ(node->display_name, "🤪 Node With Emojis");

    bool broken_float = false;
    bool weird_combo = false;
    bool any_pin = false;

    for (const auto& p : node->required.pins)
    {
        if (p->name == "broken_float")
        {
            broken_float = true;
            EXPECT_EQ(p->type, data::PinType::FLOAT);
            EXPECT_DOUBLE_EQ(std::get<double>(p->def), 0.0);
        }
        else if (p->name == "weird_combo")
        {
            weird_combo = true;
            EXPECT_EQ(p->type, data::PinType::COMBO);
            EXPECT_GE(p->list.size(), 1);
        }
        else if (p->name == "any_pin")
        {
            any_pin = true;
            EXPECT_EQ(p->type, data::PinType::ANY);
        }
    }
    EXPECT_TRUE(broken_float);
    EXPECT_TRUE(weird_combo);
    EXPECT_TRUE(any_pin);
}

