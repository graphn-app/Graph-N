#pragma once

namespace ui
{

class OptRightPane
{
public:
    OptRightPane() = default;
    virtual ~OptRightPane() = 0;

    NO_COPY_MOVE(OptRightPane);

    virtual void init(OptsData& /*data*/) {};
    virtual void show(OptsData& /*data*/) {};
};

inline OptRightPane::~OptRightPane() = default;

}
