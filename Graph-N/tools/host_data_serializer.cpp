/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "tools/host_data_serializer.h"

namespace blob
{

constexpr uint64_t g_version_1 = 292734122346942481;
constexpr uint64_t g_current_version = g_version_1;

size_t HostSerializer::get_min_size()
{
    return serialize({})->size();
}

size_t HostSerializer::get_max_size()
{
    return
        get_min_size() +
        MAX_PATH * 2 /* image pathes */ + 500 /* wf and node names */ + 20 /*comfyui host*/ + 500 /*settings*/;
}

Blob HostSerializer::serialize(const HostData& data)
{
#ifndef TOOLS_DETACHED_STATE
    static_assert(std::is_same_v<iid, uint32_t>, "Update instance id type");
    static_assert(std::is_same_v<wid, uint32_t>, "Update workflow id type");
#endif

    Writer wr;

    wr.push(g_current_version);

    wr.push(data.host_id);

    wr.push(static_cast<uint8_t>(data.cmd));
    wr.push(data.source_path.wstring());
    wr.push(data.dest_path.wstring());
    wr.push(data.sam_enabled);

    wr.push(data.wf_name);
    wr.push(data.workflow_id);
    wr.push(data.node_name);
    wr.push(data.node_id);

    wr.push(data.comfyui_host);
    wr.push(data.comfyui_port);

    wr.push(data.settings);

    return wr.release();
}

HostData HostSerializer::deserialize(const Blob& blob)
{
    HostData ret;
    if (blob->empty()) [[unlikely]]
        return ret;

    Reader rd(blob);

    const auto package_version = rd.pop_u64();
    if (package_version != g_current_version) [[unlikely]]
    {
        LogErrorS("Version " << package_version << " is not supported");
        return ret;
    }

    ret.host_id = rd.pop_u64();

    ret.cmd = static_cast<HostData::Command>(rd.pop_u8());
    ret.source_path = rd.pop_wstring();
    ret.dest_path = rd.pop_wstring();
    ret.sam_enabled = rd.pop_bool();

    ret.wf_name = rd.pop_string();
    ret.workflow_id = rd.pop_u32();
    ret.node_name = rd.pop_string();
    ret.node_id = rd.pop_u32();

    ret.comfyui_host = rd.pop_string();
    ret.comfyui_port = rd.pop_i32();

    ret.settings = rd.pop_string();

    return ret;
}

} // namespace blob
