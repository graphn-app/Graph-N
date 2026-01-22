
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

enum class PinType		// primitives and predefined colors only
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

    // POSE_KEYPOINT, GUIDER, DETAILER_HOOK, SEGM_DETECTOR, DETAILER_PIPE, IMAGE_BOUNDS, API_KEY_COMFY_ORG, AUTH_TOKEN_COMFY_ORG, UNIQUE_ID
    // UPSCALE_STACK, SCHEDULE, IMAGE_LIST, IMAGE_LIST_SIMPLE, TEXT_LIST, TEXT_LIST_SIMPLE, image_LIST, lora_LIST, PROMPT_LIST
    // SIMPLE_PROMPT_LIST, PK_HOOK, HOOKS, REGIONAL_PROMPTS, RGTHREE_CONTEXT, BATCH_COORDINATES, HOOK_KEYFRAMES, FLOAT_CURVE
    // SCHEDULER_FUNC, DICT, PLAYLIST, GLIGEN, GEMINI_INPUT_FILES, IPADAPTER_PARAMS, EMBEDS, WEIGHTS_STRATEGY, CROP_DATA, LIST
    // SEG_ELT, SEG_ELT_crop_region, SEG_ELT_control_net_wrapper, TRANSFORMERS_CLASSIFIER, UPSCALER, CAMERA_CONTROL, LOAD_3D_ANIMATION
    // LOAD_3D, WHISPER_PIPELINE, LOAD3D_CAMERA, LOSS_MAP, LUMA_CONCEPTS, LUMA_REF, OPENAI_INPUT_FILES, OPENAI_CHAT_CONFIG, 
    // PHOTOMAKER, PIXVERSE_TEMPLATE, UPSCALER_HOOK

    _count,

    CUSTOM,

    _first_widget = INT,
    _last_widget = COLOR,

    _none = _count + 100,
    _first = INT,
    _last = _count - 1
};

enum class ControlAfterGenerate
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

inline constexpr const char* ControlAfterGenerateStr[cast::to_u64(ControlAfterGenerate::_count)] =
{
    "fixed", "increment", "decrement", "randomize"
};

struct PinState final
{
    int32_t links = 0;          // number of pin links
    bool converted = false;     // input pin converted to widget
    
    uint32_t Pin_color = 0;     // will change if ANY pin
    PinType type = PinType::_none;  // will change if ANY pin
    std::string type_str;       // will change if ANY pin

    primitive value;            // current value, if widget
    int combo_index = 0;        // combo-list current selection
    float colors[4];            // for color widget
    ControlAfterGenerate control_after_generate_value = ControlAfterGenerate::Randomize; // current seed state
};

struct Pin final : public base::HashID
{
public:
    Pin(const Node* node, const std::string& Pin_name);
    Pin(const Node* node, size_t Pin_index);
    ~Pin() override;

    bool is_widget() const;

    static inline bool get_is_null(primitive& value) { return std::holds_alternative<std::monostate>(value); }
    static inline int64_t& get_int(primitive& value) { return std::get<int64_t>(value); }
    static inline uint64_t& get_uint(primitive& value) { return std::get<uint64_t>(value); }
    static inline double& get_float(primitive& value) { return std::get<double>(value); }
    static inline bool& get_bool(primitive& value) { return std::get<bool>(value); }
    static inline std::string& get_string(primitive& value) { return std::get<std::string>(value); }

    void initialize(const std::string& type_as_string);
    void finalize();

    static uint32_t color_to_val(const std::string& clr, bool& alpha_in_string);
    static int64_t colors_to_int(const float colors[4]);

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

    bool is_known_type = false;

    size_t hash = 0;
    std::string name;
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
    bool control_after_generate = false;

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

private:
    static const std::unordered_map<std::string, PinType> Pin_type_map;

private:
    static std::string primitive_to_string(primitive& item);
    void generate_Pin_color();
    void init_defaults();
    void precision_to_string();
    void clamp_values();
};

} // namespace data
