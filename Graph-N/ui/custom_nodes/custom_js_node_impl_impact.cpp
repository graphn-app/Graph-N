/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_js_node.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/workflow.h"
#include "core/base/comfyui_web_paths.h"
#include "ui/graph-n_flow.h"

namespace ui
{

void CustomJsNode::handle_impact_preview(wid wf_id, iid node_id, const std::string& image_name, const std::string& image_dir)
{
    const auto hash_callback = [wf_id, node_id](int32_t result, std::string&& binary)
    {
        if (result != HTTP_RESP_OK)
            return;
        auto wf = g::pWfsManager->access(wf_id, true);
        if (nullptr == wf)
            return;
        wf->apply_runtime_updates(node_id, "image", PinType::STRING, Primitive{ std::move(binary) });
    };

    const auto request = fmt::format(comfyui_http::impact_image, node_id, tools::str::url_encode(image_name), tools::str::url_encode(image_dir));
    g::pFlow->http_get(HttpRequestor::ImpactPreview, get_alive_callback(), request.c_str(), hash_callback);
}

} // namespace ui
