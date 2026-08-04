
#pragma once
#include "core/base/id.h"
#include "core/data/pin.h"

// todo: check utils

namespace data
{

struct Node : public base::HashID
{
    Node(const std::string& class_name);
    ~Node() noexcept override;

    void finalize(std::unordered_map<std::string, std::list<hid>>* models_map);
    Pin* find_pin(hid pin_hash, bool input) noexcept;

    bool has_input = false;
    struct Input final
    {
        std::vector<Pin*> pins;
        std::vector<std::string> order;
    };
    Input required;
    Input optional;
    Input hidden;

    bool is_dummy = false;
    bool has_output = false;
    bool output_node = false;
    std::vector<Pin*> output;

    bool can_bypass = true;
    bool can_mute = true;
    bool can_rename = true;

    std::string name;
    std::string node_class;
    std::string display_name;	// use name if empty
    std::string description;
    std::string category;		// full path

    std::string python_module;
    bool experimental = false;
    bool deprecated = false;
    bool api_node = false;

    float w_name = 0.0f;
    float w_pin_in_max = 0.0f;
    float w_pin_out_max = 0.0f;
    float w_widget_name_max = 0.0f;
    float w_widget_value_max = 0.0f;
    float w_fit = 0.0f;
};

} // namespace data
