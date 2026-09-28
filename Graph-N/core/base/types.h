#pragma once

using namespace std::chrono_literals;

using byte = uint8_t;
using app_version_type = uint64_t;
using hid = size_t;		// hash id for pin/node, based on node/pin name
using iid = uint32_t;	// node/pin instance id, unique for each workflow
using wid = uint32_t;   // workflow id - crc32 of uuid
using byte_buff = std::vector<byte>;

using number_t = double;
constexpr const number_t IM_NUM_MIN_WIDGET = -DBL_MAX / 2.0;
constexpr const number_t IM_NUM_MAX_WIDGET = DBL_MAX / 2.0;
constexpr const number_t NUMBER_ZERO = 0.0;
constexpr const number_t NUMBER_DEFAULT = NUMBER_ZERO;
constexpr const number_t NUMBER_STEP = 0.1;

using ImTextureIDPtr = std::shared_ptr<std::remove_pointer_t<ImTextureID>>;
using ImageData = std::shared_ptr<byte>;

using o_json = nlohmann::ordered_json;
using u_json = nlohmann::json;

constexpr double HTTP_REQUEST_LIFETIME = 30.0; // seconds
constexpr int32_t HIDDEN_VIDEO_PLAYING = 5; // seconds

constexpr int32_t HTTP_RESP_UNDEFINED    = 0;
constexpr int32_t HTTP_RESP_OK           = 200;
constexpr int32_t HTTP_RESP_FAIL         = -1;
constexpr int32_t HTTP_RESP_IN_PROGRESS  = -2;
constexpr int32_t HTTP_RESP_TIMEOUT      = 408;
constexpr int32_t HTTP_RESP_BAD_REQUEST  = 400;
constexpr int32_t HTTP_RESP_NOT_FOUND    = 404;
constexpr int32_t HTTP_RESP_NOT_ALLOWED  = 405;
constexpr uint32_t HTTP_RETRY_MAX_COUNT  = 5;

constexpr int32_t QUEUE_IMAGE_RECT = 512; // size
constexpr int32_t QUEUE_AUDIO_RECT = 20; // size
constexpr int32_t QUEUE_TEXT_RECT = 50; // size
constexpr int32_t QUEUE_IMAGE_LIFETIME = 30; // seconds
constexpr int32_t QUEUE_TT_IMAGE_LIFETIME = 1; // seconds
constexpr int32_t QUEUE_MAX_RUNS = 100; // count
constexpr int32_t QUEUE_DEAD_TASK = 5 * 60; // seconds

constexpr int32_t NODE_IMAGE_LIFETIME = 30; // seconds
constexpr int32_t NODE_IMAGE_RECT = 256; // size

constexpr double PLAYING_AUDIO_LIFETIME = 5.0; // seconds
constexpr double PLAYING_VIDEO_LIFETIME = 5.0; // seconds

constexpr float WIDGET_STRING_W = 200.0f;
constexpr float WIDGET_STRING_ML_H = 4.0f;
constexpr float WIDGET_COMBO_W = 150.0f;
constexpr float WIDGET_BOOL_W = 100.0f;
constexpr float WIDGET_NUM_W = 150.0f;
constexpr float WIDGET_COLOR_RGB_W = 240.0f;
constexpr float WIDGET_COLOR_RGBA_W = 270.0f;
constexpr float WIDGET_OUT_TEXT_MAX_H = 200.0f;

constexpr const char* TEMP_LABEL_COMPLETED      = "Completed tasks";
constexpr const char* TEMP_LABEL_IMPORTED       = "Imported";

constexpr const char* DND_FROM_NODES_LIBRARY    = "F86C637FBBC3";
constexpr const char* DND_FROM_SYSTEM           = "770C64EA9B66";
constexpr const char* DND_REORDER_QUEUE         = "79C13B4685DA";

constexpr const char* CNI_CLASS_REROUTE         = "Reroute";
constexpr const char* CNI_CLASS_PRIMITIVE       = "PrimitiveNode";
constexpr const char* CNI_CLASS_NOTE            = "Note";
constexpr const char* CNI_CLASS_MARKDOWN_NOTE   = "MarkdownNote";
constexpr const char* CNI_CLASS_LOAD_IMAGE      = "LoadImage";

constexpr const char* CAG_PIN_NAME              = "E742ADEF-156C-45DF-90EC-AA36CA21A9B2";
constexpr const char* REROUTE_SPLITTED_PIN      = "32DB533B-64D6-4F95-A90A-A50B17B59CDB";

using HttpResponseCb = std::function<void(int32_t, std::string&&)>; // http code, data

enum class HttpRequestor : uint8_t
{
    Queue_Output,
    Queue_History,
    Queue_Interrupt,
    Queue_Task,
    Sync_Init,
    Sync_Action,
    Models_All,
    Models_Model,
    Flow_Import_projects,
    Flow_Reparse_nodes,
    QPanel_Output,
    QPanel_History,
    SystemStats_Stats,
    Stats_Prompt,
    CustomWdgt_Image,
    CustomWdgt_Video,
    CustomWdgt_Audio,
    UploadMedia,
    Restart,
};

enum class DlgObj : uint8_t
{
    OptionsDlg,
    OptLeftPane,
    OptRightPane,
    OptRightPane_Sub,

    SyncProjects,

    ActivityBarL,
    ActivityBarR,

    PanelNodes,
    PanelNodes_ItemsList,
    PanelNodes_ItemInLabels,
    PanelNodes_LabelMenu,
    PanelNodeInspector,
    PanelStats,
    PanelWf,
    PanelWfList,
    PanelWfList_ItemsList,
    PanelWfList_ItemInLabels,
    PanelWfList_LabelMenu,
    PanelWfList_ItemsMenu,
    PanelTemplates,
    PanelServerLogs,
    PanelNotes,
    PanelModels,
    PanelModels_ItemsList,
    PanelModels_ItemMenu,
    PanelQueue,
    PanelQueue_ItemsList,
    PanelTextEditor,
    PanelMarkdownPreview,

    ModalMessage,

    _count
};

// in sync with DlgObj
constexpr const char* DlgObj_SID[static_cast<size_t>(DlgObj::_count)] = 
{
    ICON_LC_SETTINGS " Options###C0121D08-41AA-4BC6-90D9-DCE1D70BDE80",              // OptionsDlg: with caption
    "###08D470FA-B8EC-47F4-AE0E-55129C3CEFFC",                     // OptLeftPane
    "###EDAC954E-79AD-4E89-B9E9-68B4B24453A1",                     // OptRightPane
    "###C5757F45-AD33-4319-B436-450D8EAD813D",                     // OptRightPane_Sub

    "Sync projects with server###A43260E2-5E14-4A86-A0C3-52A94168DE46",             // SyncProjects

    "###D52D0CE1-9D0F-4EB3-899D-988485E942C2",                     // ActivityBarL
    "###5DF0E820-4463-4D92-B995-56EFFDDC0C9E",                     // ActivityBarR

    "###02EE3A92-9ED3-44AD-A285-95FD52C78BE3",                     // PanelNodes; labels ID
    "###C3BFE75C-FAA8-4ED8-9AF8-738E4DA97931",                     // PanelNodes_ItemsList
    "###4C123DFB-AB0D-40E6-B9E7-0E1C0A9E0CAF",                     // PanelNodes_ItemInLabels
    "###AEBD1833-7E82-4D33-BF50-437847CCDF23",                     // PanelNodes_LabelMenu
    "###B25FAE70-E031-4C9A-A682-8E182609536E",                     // PanelNodeInspector
    "###29C805C1-AA12-4FC2-9954-F7849E5DBFDA",                     // PanelStats
    "",                                                            // PanelWf: autogenerated
    "###0EE80880-D72D-48A6-808A-187FE7A17A71",                     // PanelWfList; labels ID
    "###438A7FE0-F047-4694-AD2A-366A823FE76F",                     // PanelWfList_ItemsList
    "###A03D43D4-794A-4B11-BB6D-93E4083D860B",                     // PanelWfList_ItemInLabels
    "###B2C13784-3F50-403A-A549-F7D817F54636",                     // PanelWfList_LabelsMenu
    "###289258A1-458B-4ED9-B3DD-F1BC2C5A9696",                     // PanelWfList_ItemsMenu
    "###3FE3C15E-401A-4732-A098-098D329E2BA0",                     // PanelTemplates
    "###10AAC62E-FEF9-4834-986C-70A5664DE9C4",                     // PanelServerLogs
    "###E25BB488-3298-4FB7-A5BD-9999C05AC2B8",                     // PanelNotes
    "###BE4E0091-6B9C-447F-B8FB-39DCDE0D6237",                     // PanelModels
    "###51B65039-85B0-4388-B521-BD616B636269",                     // PanelModels_ItemsList
    "###C6287EE2-BC97-4F8D-AE27-B39F59751D10",                     // PanelModels_ItemMenu
    "###61395CFA-BB2F-4375-B10A-634CC98E970D",                     // PanelQueue
    "###103105C5-D8D7-497F-8539-B81067A0DB1B",                     // PanelQueue_ItemsList
    "###28677371-A471-4242-AA17-A3830EC4D528",                     // PanelTextEditor
    "###D92D3506-6C6B-4613-BAB2-AEF2B33E81CE",                     // PanelMarkdownPreview

    "###84C3F1E0-4342-4A1B-9464-A011057E9FE1",                     // ModalMessage
};

constexpr ImGuiID DlgObj_IID[static_cast<size_t>(DlgObj::_count)] = 
{
    0x00,                           // OptionsDlg: with caption
    0x08D470FA,                     // OptLeftPane
    0xEDAC954E,                     // OptRightPane
    0xC5757F45,                     // OptRightPane_Sub

    0x00,                           // SyncProjects

    0xD52D0CE1,                     // ActivityBarL
    0x5DF0E820,                     // ActivityBarR

    0x02EE3A92,                     // PanelNodes; labels ID
    0xC3BFE75C,                     // PanelNodes_ItemsList
    0x4C123DFB,                     // PanelNodes_ItemInLabels
    0xAEBD1833,                     // PanelNodes_LabelsMenu
    0xB25FAE70,                     // PanelNodeInspector
    0x29C805C1,                     // PanelStats
    0x00,                                                            // PanelWf: autogenerated
    0x0EE80880,                     // PanelWfList; labels ID
    0x438A7FE0,                     // PanelWfList_ItemsList
    0xA03D43D4,                     // PanelWfList_ItemInLabels
    0xB2C13784,                     // PanelWfList_LabelsMenu
    0x289258A1,                     // PanelWfList_ItemsMenu
    0x3FE3C15E,                     // PanelTemplates
    0x10AAC62E,                     // PanelServerLogs
    0xE25BB488,                     // PanelNotes
    0xBE4E0091,                     // PanelModels
    0x51B65039,                     // PanelModels_ItemsList
    0xC6287EE2,                     // PanelModels_ItemMenu
    0x61395CFA,                     // PanelQueue
    0x103105C5,                     // PanelQueue_ItemsList
    0x28677371,                     // PanelTextEditor
    0xD92D3506,                     // PanelMarkdownPreview

    0x84C3F1E0,                     // ModalMessage
};

enum class PinType : uint16_t
{
    INT,
    FLOAT,
    NUMBER,
    BOOLEAN,
    COMBO,
    STRING,
    COLOR,	// TODO

    ANY,	// TODO
    IMAGE,	// TODO

    MODEL, MASK, LATENT, AUDIO, FLOATS, CONDITIONING, CLIP, VAE, VIDEO, CONTROL_NET, IPADAPTER, INSIGHTFACE, NOISE, SIGMAS, SAMPLER,
    CLIP_VISION, SEGS, PIPE_LINE, BUS, LORA_STACK, LORA_LIST, CONTROL_NET_STACK, CLIP_VISION_OUTPUT, BBOX, SEED,

    IMAGE_LIST, IMAGE_BOUNDS, UNIQUE_ID, IMAGE_LIST_SIMPLE, TEXT_LIST, PROMPT_LIST, PLAYLIST, LOAD_3D, LIST,
    LOAD_3D_ANIMATION, SIMPLE_PROMPT_LIST, TEXT_LIST_SIMPLE, DICT, API_KEY_COMFY_ORG, AUTH_TOKEN_COMFY_ORG, BATCH_COORDINATES, SEG_ELT, 
    FLOAT_CURVE, GEMINI_INPUT_FILES, OPENAI_INPUT_FILES, OPENAI_CHAT_CONFIG, IPADAPTER_PARAMS, EMBEDS, CROP_DATA, LOSS_MAP, GUIDER, SCHEDULE,
    LOAD3D_CAMERA, POSE_KEYPOINT, SEG_ELT_CROP_REGION, SEG_ELT_CONTROL_NET_WRAPPER, SCHEDULER_FUNC, REGIONAL_PROMPTS, WEIGHTS_STRATEGY,
    CAMERA_CONTROL, DETAILER_HOOK, PK_HOOK, HOOKS, HOOK_KEYFRAMES, DETAILER_PIPE, RGTHREE_CONTEXT, UPSCALER_HOOK, GLIGEN, UPSCALE_STACK,
    PHOTOMAKER, LUMA_CONCEPTS, LUMA_REF, PIXVERSE_TEMPLATE, 

    _count,

    CUSTOM,
    REROUTE_SPLITTED,

    _first_widget = INT,
    _last_widget = COLOR,

    _none = _count + 100,
    _first = INT,
    _last = CUSTOM
};

enum class PinShape : uint8_t
{
    Circle,
    SemiCircle,
    Square,
    Triangle,
    Diamond,
    Hexagon
};

enum class ControlAfterGenerate : uint8_t
{
    _none,

    Fixed = _none,
    Increment,
    Decrement,
    Randomize,

    _count,
    _first = _none,
    _last = _count - 1,
    _zero = _none
};

enum class OptPane : uint8_t
{
    _none,

    Application = _none,
    Connection,
    LocalServer,
    Interface,
    Diagnostics,
    KeyBindings,

    _count,
    _first = Application,
    _last = KeyBindings
};

enum ModalDlgCtrl : uint8_t
{
    _btn_none   = 0,

    BtnOK       = 0x00001,
    BtnCancel   = 0x00002,
    BtnYes      = 0x00004,
    BtnNo       = 0x00008,

    IcoErr      = 0x10000,
    IcoInfo     = 0x20000,
    IcoWarn     = 0x40000,
    IcoQues     = 0x80000,

    Continue    = 0x100000,

    _btn_count = 4,
    _ico_count = 4,
};

// update LogWriter with new elements!
enum class DlgCode : uint8_t
{
    Progress,
    Cancel,
};

enum class WidgetInput : uint8_t
{
    _none,

    Direct = _none,     // int
    Slider,             // int - def
    Drag,               // int
    Toggle,             // bool
    Switcher,           // bool
    Select,             // bool - def

    _count,
    _first = Direct,
    _last = _count - 1
};


enum class MetricLimit : uint8_t
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

enum class ActivityBarMode : uint8_t
{
    _none,

    Disable = _none,
    Smart,
    Left,
    Right,

    _first = Disable,
    _last = Right
};

enum class AppColor : uint16_t
{
    DisconnectedMenuBarBg,
    DisconnectedMenuBarText,
    OptBtnNormal,
    OptBtnHover,
    OptBtnPressed,
    MiscDlgTitleBg,
    MiscDlgBg,
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
    NodeHeaderIcon,
    NodeBorderMuted,
    NodeBgMuted,
    NodeBorderBypassed,
    NodeBgBypassed,
    GroupBg,
    StatNorm,
    StatWarn,
    StatCrit,
    StatSepCapt,
    StatSepSubtitle,
    PopupBg,
    MenubarBg,
    OptSeparator,
    OptHelpMark,
    OptHelpText,
    SelectedItem,
    RestartButton,
    UrlTextDefault,
    UrlTextHovered,
    FolderIcon,
    Starred,
    TextEditorCaption1,
    TextEditorCaption2,
    MsgBoxError,
    MsgBoxWarning,
    MsgBoxInfo,
    MsgBoxQuestion,
    ActivityBarBg,
    ActivityBarActive,
    ActivityBarInactive,
    TabActive,
    TabInactive,
    TabHovered,
    DummyNode,
    NoteNode,

    Pin_Int,
    Pin_Float,
    Pin_String,
    Pin_Boolean,
    Pin_Any,
    Pin_Combo,
    Pin_Color,
    Pin_RerouteSplitted,
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

enum class TaskResult : uint8_t
{
    None,
    InProgress,
    Success,
    Failed,
    Interrupted,
    Lost
};

enum class DbItemOptionType : uint8_t
{
    Pins,
    Nodes,
    LabelWfs,
    LabelNodes,
    StarredModels,
    StarredHistory,

    _count,
    _first = Pins,
    _last = _count - 1
};

enum class ConnectState : uint8_t
{
    Disconnected,
    Connecting,
    Connected,
};

// keep order in sync with the 'texts' array in OptRightPane::show_keys
// check ui_hotkey_is_skipped
enum class KeyTarget : uint16_t
{
    _none,

    App_First,
    App_Import,
    App_Options,
    App_Quit,
    App_TogglePanels,
    App_FullScreen,
    App_Command, // skip
    App_Restart, // skip
    App_ReservedFirst, // skip
    App_ReservedLast = App_ReservedFirst + 8, // skip
    App_Last,

    Wf_First,
    Wf_1,
    Wf_2,
    Wf_3,
    Wf_4,
    Wf_5,
    Wf_6,
    Wf_7,
    Wf_8,
    Wf_9,
    Wf_Next,
    Wf_Prev,
    Wf_ReservedFirst, // skip
    Wf_ReservedLast = Wf_ReservedFirst + 8, // skip
    Wf_Last,

    PanelOpen_First,
    PanelOpen_Nodes,
    PanelOpen_Models,
    PanelOpen_Projects,
    PanelOpen_Templates, //skip
    PanelOpen_Logs,
    PanelOpen_Stats,
    PanelOpen_Queue,
    PanelOpen_Inspector,
    PanelOpen_Notes, // skip
    PanelOpen_ReservedFirst, // skip
    PanelOpen_ReservedLast = PanelOpen_ReservedFirst + 8, // skip
    PanelOpen_Last,

    Project_First,
    Project_CreateNew,
    Project_Rename,
    Project_ReservedFirst, // skip
    Project_ReservedLast = Project_ReservedFirst + 8, // skip
    Project_Last,

    Queue_First,
    Queue_Pause,
    Queue_Interrupt,
    Queue_ReservedFirst, // skip
    Queue_ReservedLast = Queue_ReservedFirst + 8, // skip
    Queue_Last,

    Workspace_First,
    Workspace_Save,
    Workspace_Undo,
    Workspace_Redo,
    Workspace_SelAll,
    Workspace_Focus,
    Workspace_RunsInf,
    Workspace_Runs1,
    Workspace_Runs2,
    Workspace_Runs3,
    Workspace_Runs4,
    Workspace_Runs5,
    Workspace_Runs6,
    Workspace_Runs7,
    Workspace_Runs8,
    Workspace_Runs9,
    Workspace_RunBack,
    Workspace_RunFront,
    Workspace_Close,
    Workspace_NewNode,
    Workspace_ReservedFirst, // skip
    Workspace_ReservedLast = Workspace_ReservedFirst + 8, // skip
    Workspace_Last,

    Node_First,
    Node_Pin,
    Node_Collapse,
    Node_Group,
    Node_Always,
    Node_Mute,
    Node_Bypass,
    Node_Remove,
    Node_Copy,
    Node_Paste,
    Node_Dup,
    Node_Unlink,
    Node_Reroute,
    Node_AlignLeft,
    Node_AlignTop,
    Node_MoveLeft,
    Node_MoveRight,
    Node_MoveUp,
    Node_MoveDown,
    Node_ReservedFirst, // skip
    Node_ReservedLast = Node_ReservedFirst + 8, // skip
    Node_Last,

    _first = App_First,
    _last = Node_Last
};

enum class ZoomedVisibility : uint8_t
{
    None,             // no node content, no links
    Low,              // no node caption, no images, no videos, no pins, one big rect instead of widgets
    Medium,           // no texts, empty frame, no pin names
    Fade,             // apply alpha fading
    Max,              // full content
};
constexpr float ZOOMED_MAX = 1.7f;
constexpr float ZOOMED_FADE = 2.0f;
constexpr float ZOOMED_MEDIUM = 2.3f;
constexpr float ZOOMED_LOW = 3.0f;
constexpr float ZOOMED_NONE = 5.0f;

namespace w
{
extern float pin_l_offset_ro;
extern float pin_l_offset_rw;
extern float pin_r_offset;
extern float ico_circle;
extern float ico_eye;
extern float ico_max;
extern float v_spacing;
}
