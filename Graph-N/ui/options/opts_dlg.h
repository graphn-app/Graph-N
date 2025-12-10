#pragma once
#include "ui/base_dlg.h"
#include "ui/options/opts_defs.h"

namespace ui
{

class OptRightPane;

class OptionsDlgs : public BaseDlg<DlgData>
{
public:
	OptionsDlgs();
	~OptionsDlgs() override;

	void init() override;

private:
	DlgCode show(bool& focused) override;
	void show_left_pane();
	void show_right_pane();

private:
	OptsData m_opts_data;
	std::map<PaneType, std::shared_ptr<OptRightPane>> m_right_panes;
};


}