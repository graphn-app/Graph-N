
#pragma once
#include "core/managers/manager.h"
#include "core/data/node.h"

namespace data
{
struct Nodes
{
	Node* node = nullptr;
	bool is_node = false;
	std::string name;
	std::list<Nodes> children;
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

	const std::list<data::Nodes>& get_nodes() const;
	bool is_locked() const { return m_locked; }

private:
	void lock() { m_locked = true; }
	void unlock() { m_locked = false; }

private:
	bool m_locked = false;

	std::map<lid, data::Node> m_nodes;
	std::list<data::Nodes> m_nodes_tree;
	std::list<data::Nodes> m_dummy_tree;	// always empty
};

class NodesParser final
{
public:
	void parse(std::string data);
	void stop();

	bool is_parse_failed() const { return m_parse_failed; };
	bool is_parse_finished() const { return m_parse_finished; };
	std::pair<size_t, size_t> check_status(std::string& status);

private:
	bool parse_thread();
	void parse_node(const std::string& node_name, const nlohmann::json& node);

private:
	std::string m_json_data;

	std::future<bool> m_future;
	std::thread m_thread;
	std::atomic_bool m_stop_thread_flag = false;

	std::mutex m_mutex;
	std::string m_parse_status;
	size_t m_parsed_item = 0;
	size_t m_parsed_items_total = 0;

	std::atomic_bool m_parse_failed = false;
	std::atomic_bool m_parse_finished = false;
};

} // namespace mngr