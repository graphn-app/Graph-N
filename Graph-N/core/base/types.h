#pragma once

using byte = uint8_t;
using app_version_type = uint64_t;
using time_type = uint64_t;
using lid = uint32_t;	// local ID
using rid = uint64_t;	// unique random id
using hid = size_t;		// hash id, based on unique node/pin name
using iid = lid;	    // instance id, to save in json
using gid = uint64_t;	// app global id, to use in graph, based on workflow id
using byte_buff = std::vector<byte>;
using primitive = std::variant<int64_t, uint64_t, double, std::string, bool, std::monostate>;

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

enum class DlgObj
{
    _none,

    OptionsDlg,

    PanelNodes,
    PanelNodeInspector,
    PanelStats,
    PanelWf,
    PanelWfList,
    PanelTemplates,
    PanelOutput,
    PanelNotes,
    PanelModels,

    _count,
    _first = _none + 1,
    _last = _count - 1,
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

struct WfData
{
    rid id = 0;
    std::string id_str;
    std::string name;
    std::string json;
    bool open = false;
    bool fav = false;
    size_t nodes_inside = 0;
    bool deleted = false;
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



/*
NODE


|<------------------------------------------------------0------------------------------------------>|
|<------------------------------------------------------1/21--------------------------------------->|         ---->|
| 19
|                                     [             node name           ]                           |              |
|<---------------2------------------->|<--------------3/4-------------->|<-----------2------------->|         ---->| 
| 19
|     [       pin_ico    ]     [in_pin_name][    center      ][out_pin_name]     [ pin_ico    ]     |              |
|<-5->|<------6/13------>|<-7->|<---8/9--->|[<-----10------->|<---11/12--->|<-7->|<---6/13--->|<-5->|         ---->|
| 20
|     [    widget_ico    ]     [  widget_name       ][     widget_value    ]                        |              |
|<-5->|<------13-------->|<-7->|<-------14/15------>||<---------16/17----->|<-7->|<----6/13-->|<-5->|         ---->|

|     [                                            separator                                  |     |
|<-5->|<------13-------->|<-7->|<-----------------18-----------------------|<-7->|<----6/13-->|<-5->|


----------------------------------------------------------------------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------------------------------------------------------------------
##  | what                   | equal to                                                         | variable                          | when
----------------------------------------------------------------------------------------------------------------------------------------------------------------
3   | node name              | CalcTextSize(Node::name)                                         | Node::w_name                      | on parse
5   | pin left offset        | 4.0                                                              | w::pin_l_offset                   | app start
6   | icon width             | CalcTextSize(ICON_CIRCLE)                                        | w::ico_circle                     | app start
7   | pin right offset       | 4.0                                                              | w::pin_r_offset                   | app start
8   | input pin name         | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
11  | output pin name        | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
13  | icon width             | CalcTextSize(ICON_EYE)                                           | w::ico_eye                        | app start
14  | widget name            | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
16  | widget value           | CalcTextSize(Pin::value))                                        | Pin::w_value                      | parse
19  | vertical spacing       | 4.0                                                              | w::v_spacing                      | app start
20  | vertical spacing       | (19)*2                                                           |                                   | app start
----------------------------------------------------------------------------------------------------------------------------------------------------------------
9   | max input pin name     | max(!wigdet(8))                                                  | Node::w_pin_in_max                | first instance create
12  | max output pin name    | max(11)                                                          | Node::w_pin_out_max               | first instance create
15  | max widget name        | max(widget(14) + 2 * ImGui::GetStyle().ItemSpacing.x)            | Node::w_widget_name_max           | first instance create
17  | max widget value       | max(16)                                                          | Node::w_widget_value_max          | first instance create
21  | node ideal fit         | = (1 ideal)                                                      | Node::w_fit                       | first instance create
----------------------------------------------------------------------------------------------------------------------------------------------------------------
0   | available width        |                                                                  |                                   | every frame
1   | node width             | (5)*2 + (13)*2 + (7)*2 + (15) + (17)                             | NodeInstance::w_node              | every frame
2   | node name l/r margins  | (0)/2 - (4)/2                                                    | NodeInstance::w_node_name_off     | every frame
4   | node name              | CalcTextSize(Node::name)                                         | NodeInstance::w_name              | on node rename
10  | central zone           | (15) + (17) - (9) - (12)                                         | NodeInstance::w_center            | every frame
18  | hidden items separator | (0) - (5)*2 + (13)*2 + (7)*2                                     | NodeInstance:::w_separator        | every frame
----------------------------------------------------------------------------------------------------------------------------------------------------------------

- pin icon (16), widget ico (13) - take max depend on: tooltip, inspection, wf

- empty node:       (no in pins, no out pins, no wdg):     - (1) = (100)
- widgets only:     (no in pins, no out pins):             - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17)
- input only:       (no out pins, no wdg):                 - (10) = 50, (1) = (5)*2 + (13)*2 + (7)*2 + (9) + (10)
- output only:      (no in pins, no wdg):                  - (10) = 50, (1) = (5)*2 + (13)*2 + (7)*2 + (10) + (12)
- wdg and out:      (no in pins):                          - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17), (10) = (15) + (17) - (12)
- wdg and in:       (no out pins):                         - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17), (10) = (15) + (17) - (9)
- in and out:       (no wdg):                              - (10) = 10, (1) = (5)*2 + (13)*2 + (7)*2 + (9) + (10) + (12)

- if (0) < (21):           - (15) = ((0) - (5)*2 - (13)*2 - (7)*2) / 3, but >= 0
- (17) = ((0) - (5)*2 - (13)*2 - (7)*2) / 3 * 2, but >= 0
- (10) = (15) + (17) - (9) - (12), but >= 0
- (9) = ((15) + (17) / 2), but >= 0
- (12) = ((15) + (17) / 2), but >= 0
- (1) = (0)
- if (0) > (21):           - (15) = fixed
- (17) = (0) - (5)*2 - (13)*2 - (7)*2 - (15)
- (9) = fixed
- (12) = fixed
- (10) = (0) - (5)*2 - (13)*2 - (7)*2 - (9) - (12)
- (1) = (0)

*/
