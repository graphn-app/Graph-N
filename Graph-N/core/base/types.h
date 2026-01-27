#pragma once

using byte = uint8_t;
using app_version_type = uint64_t;
using time_type = uint64_t;
using lid = uint32_t;	// local ID
using hid = size_t;		// hash id, based on unique node/pin name
using iid = uint32_t;	// instance id, to save in json
using gid = uint64_t;	// app global id, to use in graph, based on workflow id
using wid = uint64_t;   // workflow id - hash id, based in uuid
using byte_buff = std::vector<byte>;
using primitive = std::variant<int64_t, uint64_t, double, std::string, bool, std::monostate>;

enum class ExcepCategory
{
    _none,

    InconsistentMap,
    InconsistentObjPtr,
    Logic,

    DirectX,
    FileSystem,
    SwitchEnum,
    ComfyUI,
    SQLite,
    UI,
    Main,
    Logger,
    Flow,
    API
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
    PanelTextEditor,
    PanelMarkdownPreview,

    _count,
    _first = _none + 1,
    _last = _count - 1,
};

enum class OptPane
{
    _none,

    ComfyUi,
    Stats,
    Wf,

    _count,
    _first = _none + 1,
    _last = _count - 1
};

enum class NodeOption
{
    _none,

    Advanced,
    Starred,

    _count,
    _first = _none + 1,
    _last = _count - 1
};

enum DlgCtrl
{
    _none = 0,

    BtnOK      = 0x00001,
    BtnCancel  = 0x00002,
    BtnYes     = 0x00004,
    BtnNo      = 0x00008,

    IcoErr     = 0x10000,
    IcoInfo    = 0x20000,
    IcoWarn    = 0x40000,
    IcoQues    = 0x80000,

    _btn_count = 4,
    _ico_count = 4,
};

// update LogWriter with new elements!
enum class DlgCode
{
    Progress,
    Cancel,
};

enum class WidgetInput
{
    _none,

    Direct,     // int
    Slider,     // int - def
    Drag,       // int
    Toggle,     // bool
    Switcher,   // bool
    Select,     // bool - def

    _count,
    _first = _none + 1,
    _last = _count - 1,
};


enum class MetricLimit
{
    LocalRAM,
    LocalDisk,
    LocalFPS,
    ServerRAM,
    ServerVRAM,
    ServerPyTorch,
    ServerPing,

    _count
};

enum class AppColor
{
    DisconnectedMenuBarBg,
    DisconnectedMenuBarText,
    OptBtnNormal,
    OptBtnHover,
    OptBtnPressed,
    OptDlgTitleBg,
    OptDlgBg,
    PanelBg,
    PanelActiveWfTab,
    PanelActiveHoveredWfTab,
    PanelNodesHoverGroup,
    PanelNodesHoverNode,
    PanelNodesSelNode,
    PanelNodesInspDescrFrame,
    NodeHeaderBg,
    NodeHeaderText,
    NodeBodyBg,
    NodePinText,
    NodeBorder,
    NodeWidgetHiddenSep,
    NodeDescription,
    StatNorm,
    StatWarn,
    StatCrit,
    StatSepCapt,
    StatSepSubtitle,
    PopupBg,
    MenubarBg,
    OptSubtitleSep,
    OptSeparator,
    OptHelpMark,
    OptHelpText,
    SelectedItem,
    RestartButton,
    UrlTextDefault,
    UrlTextHovered,
    Starred,

    Pin_Int,
    Pin_Float,
    Pin_String,
    Pin_Boolean,
    Pin_Any,
    Pin_Combo,
    Pin_Color,
    Pin_Number,
    Pin_Floats,
    Pin_Seed,
    Pin_Model,
    Pin_Latent,
    Pin_Vae,
    Pin_Clip,
    Pin_Image,
    Pin_Mask,
    Pin_Conditioning,
    Pin_ClipVision,
    Pin_ControlNet,
    Pin_Audio,
    Pin_Video,
    Pin_IPAdapter,
    Pin_InsightFace,
    Pin_ControlNetStack,
    Pin_ClipVisionOutput,
    Pin_Noise,
    Pin_Sigmas,
    Pin_Sampler,
    Pin_PipeLine,
    Pin_Lora,
    Pin_Segs,
    Pin_BBox,
    Pin_Bus,

    _count
};

struct DbValue final
{
    std::string sval;
    int64_t ival = 0;

    DbValue() = default;
    ~DbValue() = default;

    DbValue(const std::string& s, int64_t i)
        : sval(s)
        , ival(i)
    {}
    DbValue(int64_t i, const std::string& s)
        : sval(s)
        , ival(i)
    {}
    DbValue(int64_t i)
        : ival(i)
    {}
    DbValue(const std::string& s)
        : sval(s)
    {}

    inline bool empty() const { return 0 == ival && sval.empty(); }
};

struct DbOption final
{
    std::string name;
    DbValue val;
};

struct WfData final
{
    std::string uuid;
    wid id = 0;     // hash of 'uuid'. Curr session only. Don't save info db

    std::string name;

    std::string json;

    uint32_t history_pos = 0;
    uint32_t history_steps = 0;

    bool deleted = false;
    bool keep = false;

    std::string created_at;
    std::string saved_at;

    std::set<std::string> labels;
    std::string labels_raw;

    bool ro = false;
};

struct WfInfo final
{
    wid id = 0;
    std::string uuid;
    std::string name;
};

struct PanelListItem final
{
    int32_t depth = 0;

    bool is_separator = false;
    bool dbl_check_if_selected = false;

    bool selected = false;
    
    std::string name;
    std::string name_backup;
    bool renaming = false;
    bool need_focus = false;

    const char* icon = nullptr;

    bool can_expand = false;    // exclusive ability
    bool expanded = false;

    bool icon_active = false;
    bool icon_can_click = false;

    bool can_rclick = false;
    bool can_lclick = false;

    void* ptr = nullptr;    // user data
};

struct PanelItemState final
{
    size_t index = 0;
    bool rclicked = false;
    bool lclicked = false;
    bool renamed = false;
    bool icon_clicked = false;
    bool expanded = false;
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
extern float pin_l_offset_ro;
extern float pin_l_offset_rw;
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
