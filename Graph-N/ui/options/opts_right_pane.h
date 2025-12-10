#pragma once

namespace ui
{

class OptRightPane
{
public:
	OptRightPane() = default;
	virtual ~OptRightPane() = 0 {}

	virtual void init(OptsData& /*data*/) {};
	virtual void show(OptsData& /*data*/) {};
};


}