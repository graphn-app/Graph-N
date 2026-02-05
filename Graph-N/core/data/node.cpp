
#include "core/data/node.h"
#include "core/managers/settings_manager.h"

namespace data
{

Node::~Node() = default;

void Node::finalize()
{
    w_name = ImGui::CalcTextSize(display_name.empty() ? name.c_str() : display_name.c_str()).x;
}

} // namespace data
