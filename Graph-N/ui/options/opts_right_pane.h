#pragma once
#include "ui/options/opts_defs.h"

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

protected:
    void show_help_mark(const char*& out, float offset, bool same_line, const char* help_text);
    void show_help_text(const char* help_text);
};

inline OptRightPane::~OptRightPane() = default;

}
