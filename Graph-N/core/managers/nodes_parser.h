
#pragma once
#include "core/managers/nodes_manager.h"
#include "core/data/node.h"

namespace mngr
{

class NodesParser final
{
public:
    NodesParser() = default;
    ~NodesParser();

    NO_COPY_MOVE(NodesParser);

    void parse(std::string&& data);
    void stop();

    bool is_parse_failed() const { return m_parse_failed; };
    bool is_parse_finished() const { return m_parse_finished; };
    void check_status(size_t& parsed_item, size_t& parse_total);

private:
    std::shared_ptr<NodesManager> parse_thread();
    void parse_node(NodesManager& manager, const std::string& node_class, nlohmann::json& node);
    void parse_names(data::Node& node, const std::string& node_class, nlohmann::json& json_node);
    void parse_input(NodesManager& manager, data::Node& node, nlohmann::json& json_node);
    void parse_output(NodesManager& manager, data::Node& node, nlohmann::json& json_node);
    void parse_others(data::Node& node, nlohmann::json& json_node);
    void put_in_tree(NodesManager& manager, data::Node* node);
    void sort_nodes(NodesManager& manager);
    void prepare_parsers_map();
    void parse_by_key(const data::Node& node, data::Pin& pin, nlohmann::json& json, const std::string& key);
    void add_internal_nodes(NodesManager& manager);

private:
    std::string m_json_data;

    std::future<std::shared_ptr<NodesManager>> m_future;
    std::thread m_thread;
    std::atomic_bool m_stop_thread_flag = false;

    std::atomic_size_t m_parsed_item = 0;
    std::atomic_size_t m_parsed_items_total = 0;

    std::atomic_bool m_parse_failed = false;
    std::atomic_bool m_parse_finished = false;

    using setter = std::function<void(const data::Node&, data::Pin&, nlohmann::json&)>;
    std::unordered_map<std::string, setter> m_parsers;
};

} // namespace mngr
