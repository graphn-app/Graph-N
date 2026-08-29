#pragma once

///////////////////////////////////////////////////////////////////
///////      mngr                 /////////////////////////////////
///////////////////////////////////////////////////////////////////

namespace mngr
{
///////////////////////////////////////////////////////////////////
///////      mngr::DbValue        /////////////////////////////////
///////////////////////////////////////////////////////////////////

struct DbValue final
{
    std::string sval;
    int64_t ival = 0;

    DbValue() noexcept = default;
    ~DbValue() noexcept = default;

    DbValue(const std::string& s, int64_t i)
        : sval(s), ival(i)
    {}
    DbValue(int64_t i, const std::string& s)
        : sval(s), ival(i)
    {}
    DbValue(int64_t i) noexcept
        : ival(i)
    {}
    DbValue(const std::string& s)
        : sval(s)
    {}

    bool empty() const noexcept { return 0 == ival && sval.empty(); }
};

///////////////////////////////////////////////////////////////////
///////      mngr::DbAppOptionData        /////////////////////////
///////////////////////////////////////////////////////////////////

struct DbAppOptionData final
{
    std::string name;
    DbValue val;
};

///////////////////////////////////////////////////////////////////
///////      mngr::WfData                 /////////////////////////
///////////////////////////////////////////////////////////////////

struct WfData final
{
    std::string uuid;
    wid id = 0;
    std::string name;

    bool ro = false;
    bool virt = false;
    bool temp = false;

    bool valid = true;
    bool critical_failure = false;

    bool draft = true;
    bool dirty = false;

    bool saved_exist = 0;
    tools::dt saved_time;

    bool draft_exist = 0;
    tools::dt draft_time;

    bool activated = false;
    bool pending_delete = false;

    bool use_ext_time = false;
    tools::dt ext_time;
};

///////////////////////////////////////////////////////////////////
///////      Media data                   /////////////////////////
///////////////////////////////////////////////////////////////////

struct ImageInfo final
{
    int32_t width = 0;
    int32_t height = 0;
};

struct AudioInfo final
{
    float duration_sec = 0.0f;
    uint32_t sample_rate = 0;
    uint32_t channels = 0;
};

struct VideoInfo final
{
    float duration_sec = 0.0f;
    float fps = 0.0f;
    int32_t width = 0;
    int32_t height = 0;
    std::string thumbnail;
    int32_t thumbnail_id = 0;
};

///////////////////////////////////////////////////////////////////
///////      mngr::ExecTask               /////////////////////////
///////////////////////////////////////////////////////////////////

struct ExecTask final
{
    bool active = false;
    std::optional<bool> favorited;

    uint64_t group_id = 0;
    uint64_t task_id = 0;

    uint32_t run_no = 0;
    bool loaded_from_db = false;
    bool loaded_from_server = false;

    std::string prompt_id;      // comfyui execution id
    blob::Blob flow;            // task wf
    std::string flow_json_api;  // temp buffer
    std::string flow_json;      // temp buffer
    u_json api;                 // temp buffer
    u_json json;                // temp buffer
    wid open_id = 0;            // wf created from task
    std::string search_data;

    TaskResult result = TaskResult::None;

    std::string err_message;    // this and below: only if result==false
    std::string err_full;
    std::map<iid, std::set<std::string>> err_nodes;        // list of failed nodes: <node_id/<input_name>>

    enum class OutputType { Unknown, Gif, Audio, Image, Text };
    struct Output final
    {
        Output() noexcept = default;
        ~Output() noexcept
        {
            if (tools::path::exists(binary_path))
                tools::path::remove(binary_path);
        }

        bool preparing = false;
        bool ready = false;
        bool exists = false;

        OutputType type = OutputType::Unknown;
        std::string filename;
        std::string filetype;
        std::string subfolder;
        std::string format;
        std::string text;
        int32_t media_id = 0;
        int32_t media_id_tt = 0;
        std::string binary_path;
        uint64_t id = 0;

        std::optional<bool> media_valid;
        std::optional<bool> media_info_valid;
        ImageInfo image_info;
        AudioInfo audio_info;
        VideoInfo video_info;
    };
    std::list<Output> outputs;

    uint64_t time_start = 0;
    std::string time_start_s;
    uint64_t time_end = 0;
    uint64_t time_ms = 0;       // total execution time in milliseconds

    iid current_node = 0;
    int32_t nodes_done = 0;     // num of nodes finished processing
    float progress = 0.0f;      // progress in %
};
} // namespace mngr

///////////////////////////////////////////////////////////////////
///////      ui                           /////////////////////////
///////////////////////////////////////////////////////////////////

namespace ui
{
///////////////////////////////////////////////////////////////////
///////      ui::PanelListItem            /////////////////////////
///////////////////////////////////////////////////////////////////

struct PanelListItem final
{
    int32_t depth = 0;

    bool is_separator = false;
    bool is_empty = false;

    bool selected = false;

    std::string name;
    std::string name_p2;
    std::string name_backup;
    bool renaming = false;
    bool need_focus = false;

    const char* icon = nullptr;
    const char* icon_tooltip = nullptr;

    bool can_expand = false;    // exclusive ability
    bool expanded = false;

    bool icon_active = false;
    bool icon_can_click = false;

    std::function<bool(PanelListItem&, bool&)> on_hover_handler = nullptr;

    std::function<bool(PanelListItem&, bool, bool, bool, bool, bool, bool&)> on_icon_handler = nullptr;
    bool continue_processing_ico_handler = false;

    bool can_rclick = false;
    bool can_lclick = false;
    bool can_dblclick = false;

    std::function<bool(PanelListItem&, bool, bool, bool, bool, bool, bool&)> on_item_handler = nullptr;
    bool continue_processing_item_handler = false;

    uint32_t label = 0;
    void* ptr = nullptr;
    uint64_t dnd_data = 0;

    struct State
    {
        bool changed = false;
        void* ptr = nullptr;

        State() noexcept = default;
        virtual ~State() noexcept = default;
    };
    std::unique_ptr<State> item_state;
};

///////////////////////////////////////////////////////////////////
///////      ui::PanelItemState           /////////////////////////
///////////////////////////////////////////////////////////////////

struct PanelItemState final
{
    size_t index = 0;
    bool rclicked = false;
    bool rreleased = false;
    bool lclicked = false;
    bool lreleased = false;
    bool dblclicked = false;
    bool renamed = false;
    bool item_clicked = false;
    bool item_handled = false;
    bool icon_clicked = false;
    bool expanded = false;
};

///////////////////////////////////////////////////////////////////
///////      ui::KeyBinding                  /////////////////////////
///////////////////////////////////////////////////////////////////

struct KeyBinding
{
    bool enabled = true;
    KeyTarget target = KeyTarget::_none;
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool cmd = false;
    ImGuiKey key = ImGuiKey::ImGuiKey_None;
    bool node_related = false;
    bool repeatable = false;
};

} // namespace ui
