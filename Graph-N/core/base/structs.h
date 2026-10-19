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
    ObjectFlags<PinObjFlags> flags;
    static constexpr ObjectFlags<PinObjFlags> mask { PinObjFlags::Advanced };

    std::string serialize() const noexcept;
    void deserialize(const std::string& opts) noexcept;
};

struct NodeOptions final
{
    ObjectFlags<NodeObjFlags> flags;
    static constexpr ObjectFlags<NodeObjFlags> mask { NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaUploadButton, NodeObjFlags::CustomMediaRefreshButton, NodeObjFlags::CustomMediaFromSourceIn, NodeObjFlags::CustomMediaFromSourceOut };
    std::string display_source;

    std::string serialize() const noexcept;
    void deserialize(const std::string& opts) noexcept;
};

struct ModuleColor final
{
    ImU32 body_bg = 0;
    ImU32 head_bg = 0;
    ImU32 head_capt = 0;
};

struct ModuleColors final
{
    bool get_colors(hid item, ImU32& body_bg, ImU32& head_bg, ImU32& head_capt) const noexcept;
    void set_colors(const std::list<hid>& items, ImU32 body_bg, ImU32 head_bg, ImU32 head_capt);
    void delete_colors(const std::list<hid>& items);

    static void make_colors(ImVec4 initial_color, ImU32& bg_color, ImU32& head_bg, ImU32& caption, bool for_group) noexcept;

    std::string serialize() const noexcept;
    void deserialize(const std::string& opts) noexcept;

private:
    std::unordered_map<hid, ModuleColor> colors;
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
    uint32_t find_label(const std::string& name) const;

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
    bool is_gif = false;
};

struct TextInfo final
{
    std::vector<std::string> arr;
    std::string line;
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

    enum class OutputType : uint8_t { Unknown, Gif, Audio, Image, Text };
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
        std::string server_filename;
        std::string filetype;
        std::string subfolder;
        std::string format;
        int32_t media_id = 0;
        int32_t media_id_tt = 0;
        std::string binary_path;
        uint64_t id = 0;
        iid target_node = 0;

        std::optional<bool> media_valid;
        std::optional<bool> media_info_valid;
        ImageInfo image_info;
        AudioInfo audio_info;
        VideoInfo video_info;
        TextInfo text_info;
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

///////////////////////////////////////////////////////////////////
///////      StatsData                    /////////////////////////
///////////////////////////////////////////////////////////////////

struct StatsData final
{
    struct Progress final
    {
        bool sent = false;
        bool in_progress = false;
        int64_t next_attempt = 5;
    };

    Progress app_start_sent;
    Progress app_end_sent;
    Progress monitor_sent;
    Progress user_sent;
    bool all_sent() const noexcept { return app_start_sent.sent && app_end_sent.sent && monitor_sent.sent && user_sent.sent; }

    std::string customer_id;
    int64_t session_id = 0;
    std::string comfyui_ver;

    std::unordered_set<std::string> resolved_nodes;
    std::unordered_set<std::string> unresolved_nodes;
    std::unordered_set<std::string> python_modules;
};

} // namespace mngr

///////////////////////////////////////////////////////////////////
///////      ui                           /////////////////////////
///////////////////////////////////////////////////////////////////

namespace ui
{
///////////////////////////////////////////////////////////////////
///////      ui::PanelListItem / ui::PanelItemState      //////////
///////////////////////////////////////////////////////////////////

struct PanelListItemExternalData final
{
    void* ptr = nullptr;
    uint32_t label = 0;
    uint64_t dnd_data = 0;
};

struct PanelListItem final
{
    int32_t depth = 0;
    bool is_separator = false;      // draw separator only
    bool is_empty = false;          // draw empty line
    bool selected = false;          // select item
    bool expanded = false;          // expand item

    bool can_expand = false;        // is expanding allowed
    bool icon_always = false;       // always draw icon
    bool icon_can_click = false;    // is icon can be clicked

    std::string name;               // separator name, item name
    std::string name_label;         // small label next to the name, applied only for expandable items
    const char* icon = nullptr;     // icon to show if can_expand or icon_always
    const char* icon_tooltip = nullptr; // tooltip for the icon

    PanelListItemExternalData ext_data; // some data that caller might use
};

struct PanelItemEvent final
{
    int32_t index = -1;                 // item index
    bool l_clicked = false;             // item left clicked
    bool l_released = false;            // item left button released
    bool r_clicked = false;             // item right clicked
    bool r_released = false;            // item right button released
    bool icon_clicked = false;          // item's icon left clicked
    bool item_clicked = false;          // item clicked
    bool expand_toggled = false;        // item expanded/collapsed

    bool rename_completed = false;      // rename finished
    std::string new_name;               // new name is here
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

///////////////////////////////////////////////////////////////////
///////      js::CustomJsData             /////////////////////////
///////////////////////////////////////////////////////////////////

namespace data { struct Pin; }
namespace js
{

constexpr const char* VANILLA_REROUTE_CLASS                             = "Reroute";
constexpr const char* VANILLA_PRIMITIVE_NODE_CLASS                      = "PrimitiveNode";
constexpr const char* VANILLA_NOTE_CLASS                                = "Note";
constexpr const char* VANILLA_MARKDOWN_NOTE_CLASS                       = "MarkdownNote";
constexpr const char* VANILLA_CUSTOM_COMBO_CLASS                        = "CustomCombo";

constexpr const char* PYSSSSS_REROUTE_CLASS                             = "ReroutePrimitive|pysssss";
constexpr const char* PYSSSSS_REPEATER_CLASS                            = "Repeater|pysssss";

constexpr const char* RGTHREE_REROUTE_CLASS                             = "Reroute (rgthree)";
constexpr const char* RGTHREE_POWER_PRIMITIVE_CLASS                     = "Power Primitive (rgthree)";
constexpr const char* RGTHREE_FAST_BYPASSER_CLASS                       = "Fast Bypasser (rgthree)";
constexpr const char* RGTHREE_FAST_MUTER_CLASS                          = "Fast Muter (rgthree)";
constexpr const char* RGTHREE_FAST_GROUPS_BYPASSER_CLASS                = "Fast Groups Bypasser (rgthree)";
constexpr const char* RGTHREE_FAST_GROUPS_MUTER_CLASS                   = "Fast Groups Muter (rgthree)";
constexpr const char* RGTHREE_FAST_ACTIONS_BUTTON_CLASS                 = "Fast Actions Button (rgthree)";
constexpr const char* RGTHREE_BOOKMARK_CLASS                            = "Bookmark (rgthree)";
constexpr const char* RGTHREE_CONTEXT_MERGE_CLASS                       = "Context Merge (rgthree)";
constexpr const char* RGTHREE_CONTEXT_MERGE_BIG_CLASS                   = "Context Merge Big (rgthree)";
constexpr const char* RGTHREE_CONTEXT_SWITCH_CLASS                      = "Context Switch (rgthree)";
constexpr const char* RGTHREE_CONTEXT_SWITCH_BIG_CLASS                  = "Context Switch Big (rgthree)";

constexpr const char* IMPACT_SWITCH_CLASS                               = "ImpactSwitch";
constexpr const char* IMPACT_LATENT_SWITCH_CLASS                        = "LatentSwitch";
constexpr const char* IMPACT_SEGS_SWITCH_CLASS                          = "SEGSSwitch";
constexpr const char* IMPACT_INVERSED_SWITCH_CLASS                      = "ImpactInversedSwitch";
constexpr const char* IMPACT_REMOTE_INT_CLASS                           = "ImpactRemoteInt";
constexpr const char* IMPACT_REMOTE_BOOL_CLASS                          = "ImpactRemoteBoolean";
constexpr const char* IMPACT_REMOTE_FLOAT_CLASS                         = "ImpactRemoteFloat";
constexpr const char* IMPACT_REMOTE_STRING_CLASS                        = "ImpactRemoteString";
constexpr const char* IMPACT_REMOTE_TEXT_CLASS                          = "ImpactRemoteText";
constexpr const char* IMPACT_SET_BYPASS_CLASS                           = "ImpactNodeSetBypassState";
constexpr const char* IMPACT_SET_MUTE_CLASS                             = "ImpactNodeSetMuteState";
constexpr const char* IMPACT_SET_WIDGET_VALUE_CLASS                     = "ImpactSetWidgetValue";
constexpr const char* IMPACT_VALUE_RECEIVER_CLASS                       = "ImpactValueReceiver";

constexpr hid VANILLA_REROUTE                                           = tools::str::hash_stable(VANILLA_REROUTE_CLASS);
constexpr hid VANILLA_PRIMITIVE_NODE                                    = tools::str::hash_stable(VANILLA_PRIMITIVE_NODE_CLASS);
constexpr hid VANILLA_NOTE                                              = tools::str::hash_stable(VANILLA_NOTE_CLASS);
constexpr hid VANILLA_MARKDOWN_NOTE                                     = tools::str::hash_stable(VANILLA_MARKDOWN_NOTE_CLASS);
constexpr hid VANILLA_CUSTOM_COMBO                                      = tools::str::hash_stable(VANILLA_CUSTOM_COMBO_CLASS);

constexpr hid PYSSSSS_REROUTE                                           = tools::str::hash_stable(PYSSSSS_REROUTE_CLASS);
constexpr hid PYSSSSS_REPEATER                                          = tools::str::hash_stable(PYSSSSS_REPEATER_CLASS);

constexpr hid RGTHREE_REROUTE                                           = tools::str::hash_stable(RGTHREE_REROUTE_CLASS);
constexpr hid RGTHREE_POWER_PRIMITIVE                                   = tools::str::hash_stable(RGTHREE_POWER_PRIMITIVE_CLASS);
constexpr hid RGTHREE_FAST_BYPASSER                                     = tools::str::hash_stable(RGTHREE_FAST_BYPASSER_CLASS);
constexpr hid RGTHREE_FAST_MUTER                                        = tools::str::hash_stable(RGTHREE_FAST_MUTER_CLASS);
constexpr hid RGTHREE_FAST_GROUPS_MUTER                                 = tools::str::hash_stable(RGTHREE_FAST_GROUPS_MUTER_CLASS);
constexpr hid RGTHREE_FAST_GROUPS_BYPASSER                              = tools::str::hash_stable(RGTHREE_FAST_GROUPS_BYPASSER_CLASS);
constexpr hid RGTHREE_FAST_ACTIONS_BUTTON                               = tools::str::hash_stable(RGTHREE_FAST_ACTIONS_BUTTON_CLASS);
constexpr hid RGTHREE_BOOKMARK                                          = tools::str::hash_stable(RGTHREE_BOOKMARK_CLASS);
constexpr hid RGTHREE_CONTEXT_MERGE                                     = tools::str::hash_stable(RGTHREE_CONTEXT_MERGE_CLASS);
constexpr hid RGTHREE_CONTEXT_MERGE_BIG                                 = tools::str::hash_stable(RGTHREE_CONTEXT_MERGE_BIG_CLASS);
constexpr hid RGTHREE_CONTEXT_SWITCH                                    = tools::str::hash_stable(RGTHREE_CONTEXT_SWITCH_CLASS);
constexpr hid RGTHREE_CONTEXT_SWITCH_BIG                                = tools::str::hash_stable(RGTHREE_CONTEXT_SWITCH_BIG_CLASS);

constexpr hid IMPACT_SWITCH                                             = tools::str::hash_stable(IMPACT_SWITCH_CLASS);
constexpr hid IMPACT_LATENT_SWITCH                                      = tools::str::hash_stable(IMPACT_LATENT_SWITCH_CLASS);
constexpr hid IMPACT_SEGS_SWITCH                                        = tools::str::hash_stable(IMPACT_SEGS_SWITCH_CLASS);
constexpr hid IMPACT_INVERSED_SWITCH                                    = tools::str::hash_stable(IMPACT_INVERSED_SWITCH_CLASS);
constexpr hid IMPACT_REMOTE_INT                                         = tools::str::hash_stable(IMPACT_REMOTE_INT_CLASS);
constexpr hid IMPACT_REMOTE_BOOL                                        = tools::str::hash_stable(IMPACT_REMOTE_BOOL_CLASS);
constexpr hid IMPACT_REMOTE_FLOAT                                       = tools::str::hash_stable(IMPACT_REMOTE_FLOAT_CLASS);
constexpr hid IMPACT_REMOTE_STRING                                      = tools::str::hash_stable(IMPACT_REMOTE_STRING_CLASS);
constexpr hid IMPACT_REMOTE_TEXT                                        = tools::str::hash_stable(IMPACT_REMOTE_TEXT_CLASS);
constexpr hid IMPACT_SET_BYPASS                                         = tools::str::hash_stable(IMPACT_SET_BYPASS_CLASS);
constexpr hid IMPACT_SET_MUTE                                           = tools::str::hash_stable(IMPACT_SET_MUTE_CLASS);
constexpr hid IMPACT_SET_WIDGET_VALUE                                   = tools::str::hash_stable(IMPACT_SET_WIDGET_VALUE_CLASS);
constexpr hid IMPACT_VALUE_RECEIVER                                     = tools::str::hash_stable(IMPACT_VALUE_RECEIVER_CLASS);


struct Data {};
struct CustomCombo_Data : public Data
{
    std::unique_ptr<data::Pin> pins;
};

} // namespace js
