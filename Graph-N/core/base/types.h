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
    // !! update gopt_def_flow_clrs
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


// global identifiers
//#define GLOBAL_ID_DATE_PICKER_ID_FIRST		0x1000000
//#define GLOBAL_ID_DATE_PICKER_ID_NEXT		0x1000004
//#define GLOBAL_ID_DATE_PICKER_ID_LAST		0x1000054

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

