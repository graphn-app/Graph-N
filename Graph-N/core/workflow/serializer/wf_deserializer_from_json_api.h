/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;

class WfDeserializerFromJsonApi final
{
public:
    WfDeserializerFromJsonApi(Workflow* wf, const o_json& j);
    ~WfDeserializerFromJsonApi() = default;

    NO_COPY_MOVE(WfDeserializerFromJsonApi);

    bool deserialize();

private:
    struct ApiNode final
    {
        std::string node_id;
        iid node_iid = 0;
        std::string class_type;
        hid node_hash = 0;
        std::string title;
        o_json jinputs;
        NodeInstance* node = nullptr;

        struct Input final
        {
            std::string pin_name;
            iid pin_iid = 0;
            bool is_linked = true;
            Primitive raw_input;
            std::pair<std::string, int32_t> linked_input; // source node, source pin index
        };
        std::list<Input> inputs;

        bool invalid_definition = false;
        bool unrecognized_class = false;
        bool unrecognized_definition = false;
        std::string dump;
    };

    void create_api_node(ApiNode& api_node);
    void check_api_node_definition(ApiNode& api_node);
    void link_api_nodes(const std::list<ApiNode>& api_nodes);
    void arrange_nodes();

    void deserialize_custom_input(ApiNode& api_node);
    void deserialize_custom_input_rgthree_power_lora_loader(ApiNode& api_node);

private:
    Workflow* m_wf = nullptr;
    const o_json& m_json;
};

} // namespace data
