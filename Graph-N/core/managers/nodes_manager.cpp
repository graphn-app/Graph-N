
#include "core/managers/nodes_manager.h"
#include "core/data/node.h"

namespace mngr
{

//////////////////////////////////////////////////////////////////
//////////    MANAGER          ///////////////////////////////////
//////////////////////////////////////////////////////////////////

void NodesManager::free()
{
	m_nodes.clear();
	m_nodes_tree.clear();
}

const std::list<data::Nodes>& NodesManager::get_nodes() const 
{ 
	if (is_locked())
		return m_dummy_tree;
	return m_nodes_tree; 
}

//////////////////////////////////////////////////////////////////
//////////    PARSER           ///////////////////////////////////
//////////////////////////////////////////////////////////////////

std::pair<size_t, size_t> NodesParser::check_status(std::string& status)
{
	using namespace std::chrono_literals;
	if (!m_parse_finished && m_future.wait_for(1ms) == std::future_status::ready)
	{
		if (m_thread.joinable())
			m_thread.join();

		m_parse_failed = !m_future.get();
		if (!m_parse_failed)
		{
			m_stop_thread_flag = false;
			Log("Parsed: ok");
		}
		else
		{
			g::pNodesManager->free();			
			Log("Parsed: fail");
		}
		g::pNodesManager->unlock();
		m_parse_finished = true;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	status = m_parse_status;
	return std::make_pair(m_parsed_item, m_parsed_items_total);
}

void NodesParser::parse(std::string data)
{
	stop();
	m_parse_finished = false;
	m_parse_failed = false;
	m_stop_thread_flag = false;
	m_json_data = std::move(data);

	g::pNodesManager->lock();
	g::pNodesManager->free();

	std::lock_guard<std::mutex> lock(m_mutex);
	m_parsed_item = 0;
	m_parsed_items_total = 0;
	m_parse_status = "Parsing...";

	std::packaged_task<bool()> task(std::bind(&NodesParser::parse_thread, this));
	m_future = task.get_future();
	m_thread = std::thread(std::move(task));
}

void NodesParser::stop()
{
	m_stop_thread_flag = true;

	if (m_thread.joinable())
		m_thread.join();

	g::pNodesManager->unlock();
}

bool NodesParser::parse_thread()
{
	using json = nlohmann::json;
	try
	{
		const auto nodes = json::parse(m_json_data);
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_parsed_items_total = nodes.size();
		}

		const auto items = nodes.items();
		for (auto&[node_name, node_info] : items)
		{
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_parsed_item++;
				m_parse_status = node_name;
			}

			parse_node(node_name, node_info);

			if (m_stop_thread_flag)
			{
				Log("Parsing thread aborted");
				std::lock_guard<std::mutex> lock(m_mutex);
				m_parse_status = "Aborted";
				return false;
			}
		}
	}
	catch (json::exception& e)
	{
		LogExcep("Parse error " << e.what());

		std::lock_guard<std::mutex> lock(m_mutex);
		m_parse_status = "Parse failed";
	}
	return false;
}

void NodesParser::parse_node(const std::string& /*node_name*/, const nlohmann::json& /*node*/)
{
	using json = nlohmann::json;
}

} // namespace mngr