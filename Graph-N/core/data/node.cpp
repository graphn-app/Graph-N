
#include "core/data/node.h"
#include "core/managers/settings_manager.h"

namespace data
{

Node::~Node() = default;

void Node::finalize()
{
    w_name = ImGui::CalcTextSize(display_name.empty() ? name.c_str() : display_name.c_str()).x;

    // load options
    auto opt_fav = g::pOptsManager->node_starred_load(get_id());
    if (opt_fav.has_value())
        starred = opt_fav.value();
}

} // namespace data
