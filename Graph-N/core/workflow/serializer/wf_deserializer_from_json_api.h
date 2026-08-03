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
        std::string title;

        struct Input final
        {
            std::string pin_name;
            iid pin_iid = 0;
            bool is_linked = true;
            Primitive raw_input;
            std::pair<std::string, int32_t> linked_input; // source node, source pin index
        };
        std::list<Input> inputs;
    };

    NodeInstance* create_api_node(ApiNode& api_node, bool dummy);
    void link_api_nodes(const std::list<ApiNode>& api_nodes);

private:
    Workflow* m_wf = nullptr;
    const o_json& m_json;
    ImRect m_node_pos;
};

} // namespace data
