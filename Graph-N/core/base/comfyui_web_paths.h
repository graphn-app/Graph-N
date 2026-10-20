/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace comfyui_http
{
constexpr const char* object_info       = "/object_info";           // get
constexpr const char* embeddings        = "/embeddings";            // get
constexpr const char* extensions        = "/extensions";            // get
constexpr const char* system_stats      = "/system_stats";          // get
constexpr const char* view              = "/view";                  // get
constexpr const char* view_             = "/view?filename={}&subfolder={}&type={}";        // get
constexpr const char* prompt            = "/prompt";                // get, post
constexpr const char* queue             = "/queue";                 // get, post
constexpr const char* interrupt         = "/interrupt";             // post
constexpr const char* free              = "/free";                  // post
constexpr const char* models            = "/api/experiment/models"; // get
constexpr const char* model_meta        = "/api/view_metadata";     // get
constexpr const char* wfs_all_names     = "/api/userdata?dir=workflows&recurse=true&split=false&full_info=true";         // get
constexpr const char* wf_download_      = "/api/userdata/workflows%2F{}";                                                // get
constexpr const char* wf_upload_        = "/api/userdata/workflows%2F{}?overwrite=true&full_info=true";                  // post
constexpr const char* history_          = "/api/history?max_items={}";                                                   // get
constexpr const char* view_input        = "/api/view?type=input";                                                        // get
constexpr const char* view_input_       = "/api/view?type=input&filename={}&subfolder={}";                               // get
constexpr const char* upload_media      = "/api/upload/image";                                                           // post
constexpr const char* upload_mask       = "/api/upload/mask";                                                            // post
constexpr const char* refresh_inputs    = "/internal/files/input";                                                       // get
constexpr const char* refresh_outputs   = "/internal/files/output";                                                      // get
constexpr const char* reboot            = "/api/manager/reboot";                                                         // get

constexpr const char* clipbuffer_subfolder = "clipspace";
}

namespace comfyui_ws
{
constexpr const char* root              = "/ws?clientId={}";
}
