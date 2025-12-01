
#pragma once

namespace platform
{
class MainRoutine;
class Textures;
}

namespace mngr
{
class Manager;
class SettingsManager;
class DbManager;
class DataManager;
class NodesManager;
}

namespace ui
{
class GraphN_Flow;
}

class GraphN final
{
public:
	GraphN();
	~GraphN();

	enum class Flow
	{
		init,
		graph_n,

		exit
	};

	bool init();
	bool flow(Flow& flow_step);

	int override_exit_code();
	void exit_graphn();

public:
	// main wnd handlers
	void handle_wnd_minimized();
	void handle_wnd_restored();
	void handle_wnd_on_close();
	void handle_1sec_timer();

private:
	// flow
	bool init(Flow& flow_step);
	bool run_graphn(Flow& flow_step);

private:
	Flow m_current_flow_step = Flow::init;

	std::unique_ptr<ui::GraphN_Flow> m_graphn_flow;

	int m_exit_code = 0;
	bool m_quit = false;

	std::vector<std::unique_ptr<mngr::Manager>> m_managers;
};


namespace g
{
extern platform::MainRoutine* pMain;
extern platform::Textures* pTextures;

extern GraphN* pGraphN;
extern ui::GraphN_Flow* pFlow;

extern mngr::SettingsManager* pOptsManager;
extern mngr::DbManager* pDbManager;
extern mngr::DataManager* pDataManager;
extern mngr::NodesManager* pNodesManager;
}
