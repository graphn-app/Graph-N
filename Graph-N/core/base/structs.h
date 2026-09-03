#pragma once

///////////////////////////////////////////////////////////////////
///////      mngr                 /////////////////////////////////
///////////////////////////////////////////////////////////////////

namespace mngr
{
///////////////////////////////////////////////////////////////////
///////      Item options     /////////////////////////////////
///////////////////////////////////////////////////////////////////

struct PinOptions final
{
    bool is_advanced = false;

    std::string serialize() const noexcept;
    void deserialize(const std::string& opts) noexcept;
};

struct NodeOptions final
{
    bool display_image = false;
    bool display_video = false;
    bool display_audio = false;
    bool can_upload = false;
#ifdef _DEBUG
    bool restrict_widget = false;
#endif

    std::string serialize() const noexcept;
    void deserialize(const std::string& opts) noexcept;
};

struct Label final
{
    uint32_t id = 0;
    std::string name;
    bool auto_label = false;
    std::set<hid> members;
};

struct LabelsList final
{
    const Label* get_label(uint32_t label_id) const;
    const std::list<Label>& get_label_list() const noexcept { return labels; };

    uint32_t add_label(bool auto_label= false, std::string name = "");
    bool rename_label(uint32_t label_id, const std::string& new_name);
    void remove_label(uint32_t label_id) { std::erase_if(labels, [label_id](const Label& label) { return label_id == label.id; }); }

    bool has_item(uint32_t label_id, hid item) const;
    void add_item(uint32_t label_id, hid item);
    void remove_item(uint32_t label_id, hid item);
    void remove_item_for_all(hid item);

    std::string serialize() const noexcept;
    void deserialize(const std::string& opts) noexcept;

private:
    Label* get_label(uint32_t label_id);
    void sort_labels();

private:
    std::list<Label> labels;
    uint32_t last_label_id = 0;
};

struct StarredList final
{
    bool has_item(hid item) const { return starred.contains(item); }
    bool has_item(const std::string& item) const { return starred.contains(tools::str::hash_stable(item)); }
    bool add_item(hid item) { return starred.insert(item).second; }
    bool add_item(const std::string& item) { return starred.insert(tools::str::hash_stable(item)).second; }
    bool remove_item(hid item) { return starred.erase(item) > 0; }
    bool remove_item(const std::string& item) { return starred.erase(tools::str::hash_stable(item)) > 0; }
    void toggle_item(hid item) { if (has_item(item)) remove_item(item); else add_item(item); }
    void toggle_item(const std::string& item) { if (has_item(item)) remove_item(item); else add_item(item); }

    size_t count() const noexcept { return starred.size(); }

    std::string serialize() const noexcept;
    void deserialize(const std::string& opts) noexcept;

private:
    std::set<hid> starred;
};

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
