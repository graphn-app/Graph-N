
#pragma once
#include "core/base/id.h"
#include "core/data/pin.h"

// todo: check utils

namespace data
{

struct Node : public base::HashID
{
    using base::HashID::HashID;
    ~Node() override;

    bool has_input = false;
    struct Input final
    {
        std::vector<Pin*> pins;
        std::vector<std::string> order;
    };
    Input required;
    Input optional;
    Input hidden;

    bool has_output = false;
    bool output_node = false;
    std::vector<Pin*> output;

    std::string name;
    std::string node_class;		// if empty - matches to name
    std::string display_name;	// use name if empty
    std::string description;
    std::string category;		// full path

    std::string python_module;
    bool experimental = false;
    bool deprecated = false;
    bool api_node = false;

    bool node_width_ready = false;
    float node_width = 0.0f;
    float node_name_width = 0.0f;
    float left_group_width = 0.0f;
    float right_group_width = 0.0f;
    float max_node_widget_name_width = 0.0f;
    float max_node_widget_value_width = 0.0f;
};

} // namespace data
