#pragma once

using byte = uint8_t;
using app_version_type = uint64_t;
using time_type = uint64_t;
using lid = uint32_t;	// local ID
using rid = uint64_t;	// unique id based on random
using hid = size_t;		// hash id, based on unique node/pin name
using iid = lid;	    // instance id, to save in json
using gid = uint64_t;	// app global id, to use in graph, based on workflow id
using byte_buff = std::vector<byte>;
using primitive = std::variant<int64_t, uint64_t, double, std::string, bool, std::monostate>;

enum class VisibleObject
{
    _none,

    DisconnectedMenuBar,
    DisconnectedText,
    QueueButton,
    QueueText,
    OptionsDlg,
    OptionsBg,

    PanelNodes,
    PanelNodesHoverGroup,
    PanelNodesHoverNode,
    PanelNodesSelectedNode,

    PanelNodeInspector,
    PanelNodeInspectorDescrFrame,
    PanelStats,

    WfNodeHeaderBg,
    WfNodeHeaderText,
    WfNodeBodyBg,
    WfNodeWidgetPin,
    WfNodePinText,
    WfNodeBorder,

    _count,
    _first = _none + 1,
    _last = _count - 1,
};

enum class ExcepCategory
{
    _none,
    inconsistent_map,
    inconsistent_ptr,
    directx,
    filesystem,
    switch_enum,
    comfyui,
    sqlite,
    ui,
    main,
    logic,
    logger,
    flow,
    api
};

enum DlgCtrl
{
    //_none = 0,

    btn_ok      = 0x00001,
    btn_cancel  = 0x00002,
    btn_yes     = 0x00004,
    btn_no      = 0x00008,

    ico_err     = 0x10000,
    ico_info    = 0x20000,
    ico_warn    = 0x40000,
    ico_ques    = 0x80000,

    //_btn_count = 4,
    //_ico_count = 4,
};

// update LogWriter with new elements!
enum class DlgCode
{
    progress,

    ok,
    cancel,
    yes,
    no,
};

enum class WidgetInput
{
    _none,

    direct,     // int
    slider,     // int - def
    drag,       // int
    toggle,     // bool
    switcher,   // bool
    select,     // bool - def

    _count,
    _first = _none + 1,
    _last = _count - 1,
};


enum class MetricLimit
{
    local_ram,
    local_disk,
    local_fps,
    server_ram,
    server_vram,
    server_pytorch,
    server_ping,

    _count
};

enum class AppColor
{
    disconnected_menubar_bg,
    disconnected_menubar_text,
    opt_btn_normal,
    opt_btn_hover,
    opt_btn_pressed,
    opt_dlg_title_bg,
    opt_dlg_bg,
    panel_bg,
    panel_nodes_hover_group,
    panel_nodes_hover_node,
    panel_nodes_sel_node,
    panel_node_insp_descr_frame,
    node_header_bg,
    node_header_text,
    node_body_bg,
    node_pin_text,
    node_border,
    node_widget_hidden_sep,
    stat_norm,
    stat_warn,
    stat_crit,
    stat_sep_capt,
    stat_sep_subtitle,
    popup_bg,
    menubar_bg,
    opt_subtitle_sep,
    opt_separator,
    opt_selected_item,
    opt_help_mark,
    opt_help_text,
    pin_int,
    pin_float,
    pin_string,
    pin_boolean,
    pin_any,
    pin_combo,
    pin_color,
    pin_number,
    pin_floats,
    pin_seed,
    pin_model,
    pin_latent,
    pin_vae,
    pin_clip,
    pin_image,
    pin_mask,
    pin_conditioning,
    pin_clip_vision,
    pin_control_net,
    pin_audio,
    pin_video,
    pin_ipadapter,
    pin_insightface,
    pin_control_net_stack,
    pin_clip_vision_output,
    pin_noise,
    pin_sigmas,
    pin_sampler,
    pin_pipe_line,
    pin_lora_stack,
    pin_lora_list,
    pin_segs,
    pin_bbox,
    pin_bus,
    restart_button,
    url_text_default,
    url_text_hovered,

    _count
};

namespace comfyui_api
{
constexpr const char* object_info       = "/object_info";           // get
constexpr const char* embeddings        = "/embeddings";            // get
constexpr const char* extensions        = "/extensions";            // get
constexpr const char* history           = "/history";               // get
constexpr const char* system_stats      = "/system_stats";          // get
constexpr const char* view              = "/view";                  // get
constexpr const char* prompt            = "/prompt";                // get, post
constexpr const char* queue             = "/queue";                 // get, post
constexpr const char* interrupt         = "/interrupt";             // post
constexpr const char* free              = "/free";                  // post
constexpr const char* upload_image      = "/upload/image";          // post
constexpr const char* upload_mask       = "/upload/mask";           // post
}

namespace w
{
extern float pin_l_offset;
extern float pin_r_offset;
extern float ico_circle;
extern float ico_eye;
extern float ico_max;
extern float v_spacing;
extern std::string hidden_widgets;
extern float hidden_widgets_len;
}

