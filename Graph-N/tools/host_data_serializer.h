#pragma once

namespace blob
{

struct HostData final
{
    enum Command
    {
        Cmd_None,
        Cmd_Ready,          // host -> server. Host is ready to receive commands
        Cmd_MaskEditor,     // server -> host. Open MaskEditor
        Cmd_PosEditor,      // server -> host. Open 3dPosEditor
        Cmd_Terminate,      // server -> host. Terminate editor
        Cmd_Rename,         // server -> host. Node renamed
        Cmd_Raise,          // server -> host. Bring host window to foreground
        Cmd_Apply,          // host -> server. Dest image saved
    };

    Command cmd = Cmd_None;
    uint64_t host_id = 0;                       // all commands. Host should ignore command if ID is not match

    std::filesystem::path source_path;          // MaskEditor, PosEditor                -> source image path
    std::filesystem::path dest_path;            // MaskEditor, PosEditor, Apply         -> path to dest image prepared file
    bool sam_enabled = false;                   // MaskEditor                           -> SAM on/off

    std::string wf_name;                        // MaskEditor, PosEditor                -> workflow name
    uint32_t workflow_id = 0;                   // MaskEditor, PosEditor, Apply         -> workflow id
    std::string node_name;                      // MaskEditor, PosEditor, Rename        -> node name
    uint32_t node_id = 0;                       // MaskEditor, PosEditor, Apply         -> node id

    std::string comfyui_host;                   // MaskEditor                           -> ComfyUI server host, required if SAM is ON
    int32_t comfyui_port = 0;                   // MaskEditor                           -> ComfyUI server port, required if SAM is ON

    std::string settings;                       // MaskEditor, PosEditor, Apply         -> send saved/updated settings

    // internal, do not serialize
    bool alive = false;
    bool cleanup = false;
    bool can_upload_to_node = true;
};

class HostSerializer
{
public:
    static Blob serialize(const HostData& data);
    static HostData deserialize(const Blob& blob);

    static size_t get_min_size();
    static size_t get_max_size();
};

} // namespace blob
