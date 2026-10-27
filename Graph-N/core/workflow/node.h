/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/base/id.h"
#include "core/workflow/pin.h"

// todo: check utils

namespace data
{

struct Node final : public base::HashID
{
    Node(const std::string& class_name);
    ~Node() noexcept override;

    void finalize(std::unordered_map<std::string, std::list<hid>>* models_map);
    Pin* find_pin(hid pin_hash, bool input) noexcept;

    ObjectFlags<NodeObjFlags> flags;

    std::vector<Pin*> input;
    std::vector<Pin*> output;

    Pin* custom_widget_source = nullptr;
    Pin* base64_widget_dest = nullptr;

    std::string name;
    std::string node_class;
    std::string display_name;	// use name if empty
    std::string description;
    std::string category;		// full path
    std::string python_module;
    bool python_module_custom_node = false;
    hid python_module_hash = 0;
    std::string search_aliases;
    std::string built_subcapt;

    float w_name = 0.0f;
    float w_pin_in_max = 0.0f;
    float w_pin_out_max = 0.0f;
    float w_widget_name_max = 0.0f;
    float w_widget_value_max = 0.0f;
    float w_fit = 0.0f;
};

} // namespace data
