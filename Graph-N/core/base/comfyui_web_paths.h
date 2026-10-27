/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace comfyui_http
{
inline constexpr const char* object_info       = "/object_info";           // get
inline constexpr const char* embeddings        = "/embeddings";            // get
inline constexpr const char* extensions        = "/extensions";            // get
inline constexpr const char* system_stats      = "/system_stats";          // get
inline constexpr const char* view              = "/view";                  // get
inline constexpr const char* view_             = "/view?filename={}&subfolder={}&type={}";        // get
inline constexpr const char* prompt            = "/prompt";                // get, post
inline constexpr const char* queue             = "/queue";                 // get, post
inline constexpr const char* interrupt         = "/interrupt";             // post
inline constexpr const char* free              = "/free";                  // post
inline constexpr const char* models            = "/api/experiment/models"; // get
inline constexpr const char* model_meta        = "/api/view_metadata";     // get
inline constexpr const char* wfs_all_names     = "/api/userdata?dir=workflows&recurse=true&split=false&full_info=true";         // get
inline constexpr const char* wf_download_      = "/api/userdata/workflows%2F{}";                                                // get
inline constexpr const char* wf_upload_        = "/api/userdata/workflows%2F{}?overwrite=true&full_info=true";                  // post
inline constexpr const char* history_          = "/api/history?max_items={}";                                                   // get
inline constexpr const char* view_input        = "/api/view?type=input";                                                        // get
inline constexpr const char* view_input_       = "/api/view?type=input&filename={}&subfolder={}";                               // get
inline constexpr const char* upload_media      = "/api/upload/image";                                                           // post
inline constexpr const char* upload_mask       = "/api/upload/mask";                                                            // post
inline constexpr const char* refresh_inputs    = "/internal/files/input";                                                       // get
inline constexpr const char* refresh_outputs   = "/internal/files/output";                                                      // get
inline constexpr const char* reboot            = "/api/manager/reboot";                                                         // get
inline constexpr const char* impact_image      = "/api/impact/set/pb_id_image?node_id={}&filename={}&type=input&subfolder={}";  // get

inline constexpr const char* clipbuffer_subfolder = "clipspace";
}

namespace comfyui_ws
{
inline constexpr const char* root              = "/ws?clientId={}";
}
