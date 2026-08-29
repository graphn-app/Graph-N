#pragma once
#include "core/base/id.h"
#include "core/workflow/pin.h"

// todo: check utils

namespace data
{

struct Node : public base::HashID
{
    Node(const std::string& class_name);
    ~Node() noexcept override;

    void finalize(std::unordered_map<std::string, std::list<hid>>* models_map);
    Pin* find_pin(hid pin_hash, bool input) noexcept;

    ObjectFlags<NodeObjFlags> flags;

    std::vector<Pin*> input;
    std::vector<Pin*> output;

    std::string name;
    std::string node_class;
    std::string display_name;	// use name if empty
    std::string description;
    std::string category;		// full path
    std::string python_module;

    float w_name = 0.0f;
    float w_pin_in_max = 0.0f;
    float w_pin_out_max = 0.0f;
    float w_widget_name_max = 0.0f;
    float w_widget_value_max = 0.0f;
    float w_fit = 0.0f;
};

} // namespace data
