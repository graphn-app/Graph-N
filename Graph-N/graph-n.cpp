
#include "ui/graph-n_flow.h"
#include "core/managers/data_manager.h"
#include "core/managers/db_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/connect_manager.h"
#include "core/base/id.h"

namespace g
{
platform::MainRoutine* pMain = nullptr;
platform::Textures* pTextures = nullptr;
GraphN* pGraphN = nullptr;
ui::GraphN_Flow* pFlow = nullptr;
mngr::SettingsManager* pOptsManager = nullptr;
mngr::DbManager* pDbManager = nullptr;
mngr::DataManager* pDataManager = nullptr;
mngr::NodesManager* pNodesManager = nullptr;
mngr::ConnectManager* pConnectManager = nullptr;
}

GraphN::GraphN()
{
	LogF;
}

GraphN::~GraphN()
{
	LogF;
}

bool GraphN::init()
{
	base::GlobalID::get_client_id();	// initialize unique client id

	m_managers.emplace_back(std::make_unique<mngr::DbManager>());
	g::pDbManager = (mngr::DbManager*)m_managers.back().get();

	m_managers.emplace_back(std::make_unique<mngr::DataManager>());
	g::pDataManager = (mngr::DataManager*)m_managers.back().get();

	m_managers.emplace_back(std::make_unique<mngr::SettingsManager>());
	g::pOptsManager = (mngr::SettingsManager*)m_managers.back().get();

	m_managers.emplace_back(std::make_unique<mngr::NodesManager>());
	g::pNodesManager = (mngr::NodesManager*)m_managers.back().get();

	m_managers.emplace_back(std::make_unique<mngr::NodesManager>());
	g::pNodesManager = (mngr::NodesManager*)m_managers.back().get();

	m_managers.emplace_back(std::make_unique<mngr::ConnectManager>());
	g::pConnectManager = (mngr::ConnectManager*)m_managers.back().get();

	bool success = true;
	std::for_each(m_managers.begin(), m_managers.end(), [&success](std::unique_ptr<mngr::Manager>& manager)
	{
		if (success && !manager->init())
			success = false;
	});
	if (!success)
		return false;
	Log("Managers ready");

	return true;
}

void GraphN::exit_graphn()
{
	LogF;

	if (m_graphn_flow)
		m_graphn_flow->exit();
	m_graphn_flow.reset();
	g::pFlow = nullptr;

	std::for_each(m_managers.begin(), m_managers.end(), [](std::unique_ptr<mngr::Manager>& manager)
	{
		manager->stop_all_threads();
		manager->free();
	});

	g::pNodesManager = nullptr;
	g::pOptsManager = nullptr;
	g::pDataManager = nullptr;
	g::pDbManager = nullptr;
	g::pConnectManager = nullptr;
}

int GraphN::override_exit_code()
{
	return m_exit_code;
}

void GraphN::handle_wnd_minimized()
{}

void GraphN::handle_wnd_restored()
{}

void GraphN::handle_wnd_on_close()
{
	m_quit = true;
}

void GraphN::handle_1sec_timer()
{}

bool GraphN::flow(Flow& flow_step)
{
	m_current_flow_step = flow_step;

	if (m_quit)
	{
		Log("Handle exit by user");
		flow_step = Flow::exit;
		return false;
	}

	bool result = false;
	try
	{
		switch (flow_step)
		{
		case Flow::init:
			result = init(flow_step);
			break;
		case Flow::graph_n:
			result = run_graphn(flow_step);
			break;
		case Flow::exit:
            if (m_graphn_flow)
                m_graphn_flow->exit();
			break;
		default:
			LogAndThrow("Unhandler flow step " << cast::to_u32(flow_step));
		}
	}
	catch (const std::exception& ex)
	{
		result = false;
		LogAndReThrow(ex.what());
	}

	return result;
}

bool GraphN::init(Flow& flow_step)
{
	LogF;

	_tzset();

	flow_step = Flow::graph_n;
	return true;
}

bool GraphN::run_graphn(Flow& flow_step)
{
	if (nullptr == m_graphn_flow)
	{
		Log("Starting Graph-N flow");
		m_graphn_flow = std::make_unique<ui::GraphN_Flow>();
		g::pFlow = m_graphn_flow.get();
		if (!g::pFlow->init())
			return false;
	}

	const bool res = m_graphn_flow->run();
	if (!res)
	{
		Log("GraphN flow exited, next flow step: exit");
		flow_step = Flow::exit;
	}
	return res;
}


