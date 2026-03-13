
#pragma once
#include "core/base/id.h"

// TODO remove commented/unused
// TODO: output_is_list ??
// TODO: ANY
// TODO: NUMBER
// TODO: COLOR, color_to_val

namespace data
{

struct Node;

enum class PinType
{
    INT,
    FLOAT,
    BOOLEAN,
    COMBO,
    STRING,
    NUMBER,	// TODO
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

    _first_widget = INT,
    _last_widget = COLOR,

    _none = _count + 100,
    _first = INT,
    _last = _count - 1
};

enum ControlAfterGenerate
{
    _none,

    CAGFixed = _none,
    CAGIncrement,
    CAGDecrement,
    CAGRandomize,

    _count,
    _first = _none,
    _last = _count - 1,
    _zero = _none
};

inline constexpr const char* ControlAfterGenerateStr[ControlAfterGenerate::_count] =
{
    "fixed", "increment", "decrement", "randomize"
};

enum class PinShape
{
    Circle,
    SemiCircle,
    Square,
    Triangle,
    Diamond,
    Hexagon
};

struct PinState final
{
    int32_t links = 0;          // number of pin links
    bool converted = false;     // input pin converted to widget
    
    ImU32 pin_color = 0;        // will change if ANY pin
    PinType type = PinType::_none;  // will change if ANY pin
    std::string type_str;       // will change if ANY pin
    size_t type_hash = 0;

    std::string name;
    primitive value;            // current value, if widget
    iid cag = 0;
};

struct Pin final : public base::HashID
{
public:
    Pin(const Node* node, const std::string& pin_name);
    Pin(const Node* node, size_t pin_index);
    ~Pin() override;

    bool is_widget() const;

    static inline bool get_is_null(const primitive& value) { return std::holds_alternative<std::monostate>(value); }
    static inline int64_t* get_int(primitive& value) { return std::get_if<int64_t>(&value); }
    static inline uint64_t* get_uint(primitive& value) { return std::get_if<uint64_t>(&value); }
    static inline double* get_float(primitive& value) { return std::get_if<double>(&value); }
    static inline bool* get_bool(primitive& value) { return std::get_if<bool>(&value); }
    static inline std::string* get_string(primitive& value) { return std::get_if<std::string>(&value); }

    static std::string any_to_string(const primitive& item);

    void initialize(const std::string& type_as_string);
    void finalize();

    static uint64_t color_to_val(const std::string& clr, bool& alpha_in_string);
    static uint64_t colors_to_int(const ImVec4& colors);

    static PinShape get_shape(PinType type);

    static size_t get_hash(std::string str);

    const Node* parent_node = nullptr;

    bool input = false;
    bool output = false;

    bool required = false;
    bool optional = false;
    bool hidden = false;

    bool output_is_list = false;
    bool force_input = false;
    bool def_input = false; // TODO
    bool lazy = false; // TODO
    bool advanced = false;      // is visible on node

    bool image_upload = false;
    bool audio_upload = false;
    bool file_upload = false;
    bool video_upload = false;
    std::string image_folder;

    float w_name = 0.0f;
    float w_value = 0.0f;
    std::string tooltip;

    bool type_any = false;
    PinState def_state;

    // all
    primitive def;

    // string
    bool multiline = false;
    std::string placeholder;
    bool dynamic_prompts = false;

    // boolean
    std::string label_on;
    std::string label_on_named;
    std::string label_off;
    std::string label_off_named;

    // int
    bool is_unsigned = false;

    // int, double
    primitive min;
    primitive max;
    primitive step;
    bool display_as_number = false;
    bool display_as_color = false;
    bool display_as_slider = false;

    // double
    std::optional<double> round;
    std::optional<bool> do_round;
    std::string precision;
    std::string precision_named;

    // combo
    bool multiselect = false;
    std::vector<primitive> list;
    std::vector<const char*> combo_items_ptr;
    std::vector<std::string> combo_items_str;

    // color
    bool alpha_enabled = true;

    // control after generate
    bool cag_require = false;
    bool cag_pin = false;

private:
    static const std::unordered_map<std::string, PinType> pin_type_map;

private:
    void generate_pin_color();
    void init_defaults();
    void precision_to_string();
    void clamp_values();
};

} // namespace data
