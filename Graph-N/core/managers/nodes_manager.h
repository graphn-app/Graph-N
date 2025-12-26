
#pragma once
#include "core/managers/manager.h"
#include "core/data/node.h"

// TODO: remove not used

namespace data
{
struct Nodes : public base::LocalID
{
    Nodes() = default;
    ~Nodes() override = default;

	std::weak_ptr<Node> node;
	bool is_node = false;

	std::string name_or_category;
    std::string search;

	std::vector<Nodes> children;

    uint32_t nodes_inside = 0;
    uint32_t nodes_inside_unfiltered = 0;
    bool filtered = false;
};
}

namespace mngr
{

class NodesManager final : public Manager
{
	friend class NodesParser;

public:
	NodesManager() = default;
	~NodesManager() override = default;

	void free() override;

	const std::vector<data::Nodes>& get_nodes_tree() const { return m_nodes_tree; }		// must not be saved as class/global member, always do request
	const std::map<std::string, std::shared_ptr<data::Node>>& get_nodes_sorted() const { return m_nodes_sorted; }	// must not be saved as class/global member, always do request

    void calc_number_inside(const std::string& filter);

	std::shared_ptr<data::Node> find_node(lid id) const;		// returns nullptr if not found
	std::shared_ptr<data::Pin> find_pin(lid id) const;			// returns nullptr if not found

private:
	void init_from_copy(NodesManager& copy);

private:
	std::map<lid, std::shared_ptr<data::Node>> m_nodes_by_id;
	std::map<std::string, std::shared_ptr<data::Node>> m_nodes_by_name;		// map by string key
	std::map<std::string, std::shared_ptr<data::Node>> m_nodes_sorted;		// sorted by display name
	std::map<lid, std::shared_ptr<data::Pin>> m_pins;

	std::vector<data::Nodes> m_nodes_tree;
	std::vector<data::Nodes> m_dummy_tree;	// always empty
};

class NodesParser final
{
public:
	void parse(std::string data);
	void stop();

	bool is_parse_failed() const { return m_parse_failed; };
	bool is_parse_finished() const { return m_parse_finished; };
	void check_status(size_t& parsed_item, size_t& parse_total);

private:
	NodesManager parse_thread();
	void parse_node(NodesManager& manager, const std::string& node_class, nlohmann::json& node);
	void parse_names(data::Node& node, const std::string& node_class, nlohmann::json& json_node);
	void parse_input(NodesManager& manager, data::Node& node, nlohmann::json& json_node);
	void parse_output(NodesManager& manager, data::Node& node, nlohmann::json& json_node);
	void parse_others(data::Node& node, nlohmann::json& json_node);
	void put_in_tree(NodesManager& manager, std::shared_ptr<data::Node> node);
	void sort_nodes_tree(NodesManager& manager);
	void prepare_parsers_map();
	void parse_by_key(const data::Node& node, data::Pin& pin, nlohmann::json& json, const std::string& key);
	void add_internal_nodes(NodesManager& manager);

private:
	std::string m_json_data;

	std::future<NodesManager> m_future;
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