
#pragma once
#include "core/managers/nodes_manager.h"
#include "core/data/node.h"

namespace mngr
{

class NodesParser final
{
public:
    NodesParser() noexcept = default;
    ~NodesParser() noexcept;

    NO_COPY_MOVE(NodesParser);

    void parse(std::string&& data);
    void stop();

    bool is_parse_failed() const noexcept { return m_parse_failed.load(std::memory_order_relaxed); };
    bool is_parse_finished() const noexcept { return m_parse_finished.load(std::memory_order_relaxed); };
    void check_status(size_t& parsed_item, size_t& parse_total);

private:
    std::shared_ptr<NodesManager> parse_thread();
    void parse_node(NodesManager& manager, const std::string& node_class, const o_json& jnode, std::unordered_map<std::string, std::list<hid>>& models_map);
    void parse_names(data::Node& node, const std::string& node_class, const o_json& jnode);
    void parse_input(NodesManager& manager, data::Node& node, const o_json& jnode);
    void parse_output(NodesManager& manager, data::Node& node, const o_json& jnode);
    void parse_others(data::Node& node, const o_json& jnode);
    void put_in_tree(NodesManager& manager, data::Node* node);
    void sort_nodes(NodesManager& manager);
    void prepare_parsers_map();
    void parse_by_key(const data::Node& node, data::Pin& pin, const o_json& j, const std::string& key);

private:
    std::string m_json_data;

    std::future<std::shared_ptr<NodesManager>> m_future;
    std::thread m_thread;
    std::atomic_bool m_stop_thread_flag = false;

    std::atomic_size_t m_parsed_item = 0;
    std::atomic_size_t m_parsed_items_total = 0;

    std::atomic_bool m_parse_failed = false;
    std::atomic_bool m_parse_finished = false;

    using setter = std::function<void(const data::Node&, data::Pin&, const o_json&)>;
    std::unordered_map<std::string, setter> m_parsers;
};

} // namespace mngr
