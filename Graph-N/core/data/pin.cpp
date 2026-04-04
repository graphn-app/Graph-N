
#include "core/data/pin.h"
#include "core/data/node.h"
#include "core/managers/settings_manager.h"

namespace data
{

const std::unordered_map<std::string, PinType> Pin::pin_type_map =
{
    { "INT", PinType::INT },				            { "FLOAT", PinType::FLOAT },				        { "BOOLEAN", PinType::BOOLEAN },
    { "COMBO", PinType::COMBO },			            { "STRING", PinType::STRING },				        { "*", PinType::ANY },
    { "COLOR", PinType::COLOR },			            { "NUMBER", PinType::NUMBER },				        { "INSIGHTFACE", PinType::INSIGHTFACE },
    { "IMAGE", PinType::IMAGE },			            { "MODEL", PinType::MODEL },				        { "MASK", PinType::MASK },
    { "LATENT", PinType::LATENT },			            { "VAE", PinType::VAE },					        { "CONDITIONING", PinType::CONDITIONING },
    { "CLIP", PinType::CLIP },				            { "NOISE", PinType::NOISE },				        { "CONTROL_NET", PinType::CONTROL_NET },
    { "SIGMAS", PinType::SIGMAS },			            { "BUS", PinType::BUS },					        { "IPADAPTER", PinType::IPADAPTER },
    { "SAMPLER", PinType::SAMPLER },		            { "AUDIO", PinType::AUDIO },				        { "SEED", PinType::SEED },
    { "SEGS", PinType::SEGS },				            { "CLIP_VISION", PinType::CLIP_VISION },	        { "CONTROL_NET_STACK", PinType::CONTROL_NET_STACK },
    { "PIPE_LINE", PinType::PIPE_LINE },	            { "VIDEO", PinType::VIDEO },				        { "LORA_STACK", PinType::LORA_STACK },
    { "BBOX", PinType::BBOX },				            { "LORA_LIST", PinType::LORA_LIST },		        { "CLIP_VISION_OUTPUT", PinType::CLIP_VISION_OUTPUT },
    { "FLOATS", PinType::FLOATS },                      { "IMAGE_BOUNDS", PinType::IMAGE_BOUNDS },          { "UNIQUE_ID", PinType::UNIQUE_ID },
    { "IMAGE_LIST", PinType::IMAGE_LIST },              { "TEXT_LIST", PinType::TEXT_LIST },                { "IMAGE_LIST_SIMPLE", PinType::IMAGE_LIST_SIMPLE },
    { "image_LIST", PinType::IMAGE_LIST },              { "DICT", PinType::DICT },                          { "TEXT_LIST_SIMPLE", PinType::TEXT_LIST_SIMPLE },
    { "PROMPT_LIST", PinType::PROMPT_LIST },            { "LORA_LIST", PinType::LORA_LIST },		        { "SIMPLE_PROMPT_LIST", PinType::SIMPLE_PROMPT_LIST },
    { "PLAYLIST", PinType::PLAYLIST },		            { "LIST", PinType::LIST },		                    { "LOAD_3D_ANIMATION", PinType::LOAD_3D_ANIMATION },
    { "LOAD_3D", PinType::LOAD_3D },                    { "EMBEDS", PinType::EMBEDS },                      { "API_KEY_COMFY_ORG", PinType::API_KEY_COMFY_ORG },
    { "CROP_DATA", PinType::CROP_DATA },                { "LOSS_MAP", PinType::LOSS_MAP },                  { "AUTH_TOKEN_COMFY_ORG", PinType::AUTH_TOKEN_COMFY_ORG },
    { "PK_HOOK", PinType::PK_HOOK },                    { "HOOKS", PinType::HOOKS },                        { "BATCH_COORDINATES", PinType::BATCH_COORDINATES },
    { "GUIDER", PinType::GUIDER },                      { "FLOAT_CURVE", PinType::FLOAT_CURVE },            { "GEMINI_INPUT_FILES", PinType::GEMINI_INPUT_FILES },
    { "IPADAPTER_PARAMS", PinType::IPADAPTER_PARAMS },	{ "LUMA_REF", PinType::LUMA_REF },		            { "PIXVERSE_TEMPLATE", PinType::PIXVERSE_TEMPLATE },            
    { "POSE_KEYPOINT", PinType::POSE_KEYPOINT },		{ "SEG_ELT", PinType::SEG_ELT },		            { "SEG_ELT_crop_region", PinType::SEG_ELT_CROP_REGION },
    { "SCHEDULE", PinType::SCHEDULE },                  { "LUMA_CONCEPTS", PinType::LUMA_CONCEPTS },		{ "OPENAI_INPUT_FILES", PinType::OPENAI_INPUT_FILES },
    { "SCHEDULER_FUNC", PinType::SCHEDULER_FUNC },      { "REGIONAL_PROMPTS", PinType::REGIONAL_PROMPTS },	{ "WEIGHTS_STRATEGY", PinType::WEIGHTS_STRATEGY },
    { "CAMERA_CONTROL", PinType::CAMERA_CONTROL },		{ "LOAD3D_CAMERA", PinType::LOAD3D_CAMERA },		{ "DETAILER_HOOK", PinType::DETAILER_HOOK },
    { "HOOK_KEYFRAMES", PinType::HOOK_KEYFRAMES },      { "PHOTOMAKER", PinType::PHOTOMAKER },              { "SEG_ELT_control_net_wrapper", PinType::SEG_ELT_CONTROL_NET_WRAPPER },
    { "UPSCALER_HOOK", PinType::UPSCALER_HOOK },		{ "DETAILER_PIPE", PinType::DETAILER_PIPE },		{ "RGTHREE_CONTEXT", PinType::RGTHREE_CONTEXT },
    { "UPSCALE_STACK", PinType::UPSCALE_STACK },        { "GLIGEN", PinType::GLIGEN },		                { "OPENAI_CHAT_CONFIG", PinType::OPENAI_CHAT_CONFIG }
};

Pin::Pin(const Node* node, const std::string& pin_name)
    : base::HashID(node->node_class + "_IN_" + pin_name)
    , parent_node(node)
{
}

Pin::Pin(const Node* node, int64_t pin_index)
    : base::HashID(node->node_class + "_OUT_" + std::to_string(pin_index))
    , parent_node(node)
    , out_index(pin_index)
{
}

Pin::~Pin() = default;

hid Pin::make_hash(const Node* node, const std::string& pin_name, int64_t pin_index)
{
    if (-1 == pin_index)
        return base::HashID::make_hash(node->node_class + "_IN_" + pin_name);
    return base::HashID::make_hash(node->node_class + "_OUT_" + std::to_string(pin_index));
}

bool Pin::is_widget() const
{
    return !output && !force_input &&
        cast::to_i64(def_state.type) >= cast::to_i64(PinType::_first_widget) && cast::to_i64(def_state.type) <= cast::to_i64(PinType::_last_widget);
}

void Pin::initialize(const std::string& type_as_string)
{
    def_state.type_str = type_as_string;
    def_state.type = PinType::_none;
    auto map_it = pin_type_map.find(type_as_string);
    if (map_it != pin_type_map.end())
    {
        def_state.type = map_it->second;
        type_any = (PinType::ANY == def_state.type);
    }
    else
    {
        def_state.type = PinType::CUSTOM;
    }

    def_state.type_hash = get_hash(def_state.type_str);

    init_defaults();
    generate_pin_color();
}

void Pin::finalize()
{
    if (input == output)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Pin can't be input and output at the same time");
    }

    if (output && advanced)
    {
        LogInvalidJson("Output pin can't be advanced");
        LogAndThrowInternalError(ExcepCategory::Logic, "Output pin can't be advanced");
    }

    else if (input)
    {
        if (display_as_color && display_as_slider)
        {
            display_as_color = display_as_slider = false;
            LogInvalidJson("Both display_as_color and display_as_slider can't be set");
            LogAndThrowInternalError(ExcepCategory::Logic, "Both display_as_color and display_as_slider can't be set");
        }
        display_as_number = !(display_as_color || display_as_slider);

        if (!required && !optional && !hidden)
        {
            LogInvalidJson("Unknown pin visibility");
            LogAndThrowInternalError(ExcepCategory::Logic, "Unknown pin visibility");
        }

        if (PinType::FLOAT == def_state.type)
        {
            if (!std::holds_alternative<double>(step))
            {
                if (round.has_value())
                    step = round.value();
                else
                    step = double(0.01);
            }

            if (!do_round.has_value())
            {
                do_round = true;
            }

            if (!round.has_value())
            {
                if (do_round.value())
                    round = 1.0;
                else
                    round = 0.01;
            }

            precision_to_string();
        }

        else if (PinType::INT == def_state.type || PinType::NUMBER == def_state.type)
        {
            if (!std::holds_alternative<int64_t>(step) || 0 == get_int(step))
                step = int64_t(1);

            precision = fmt::format(FMT_STRING("%lld"));
            precision_named = fmt::format(FMT_STRING("{}: %lld"), def_state.name);

            const bool seed = (tools::str::to_lower_ascii(def_state.name).find("seed") != std::string::npos);
            cag_require = seed;

            if (seed || max < min)  // usually max = -1 and min = 0 -> unsigned int
            {
                is_unsigned = true;
                precision = fmt::format(FMT_STRING("%llu"));
                precision_named = fmt::format(FMT_STRING("{}: %llu"), def_state.name);
                step = uint64_t(get_int(step));
                def = uint64_t(get_int(def));
                min = uint64_t(get_int(min));
                max = uint64_t(ULLONG_MAX);
            }
        }

        else if (PinType::BOOLEAN == def_state.type)
        {
            label_on_named = fmt::format(FMT_STRING("{}: {}"), def_state.name, (label_on.empty() ? "on" : label_on));
            label_off_named = fmt::format(FMT_STRING("{}: {}"), def_state.name, (label_off.empty() ? "off" : label_off));
        }

        else if (PinType::COLOR == def_state.type && std::holds_alternative<std::string>(def))
        {
            is_unsigned = true;
            def = color_to_val(*std::get_if<std::string>(&def), alpha_enabled);
        }

        else if (PinType::COMBO == def_state.type)
        {
            if (cag_pin)
            {
                for (size_t i = 0; i < sizeof(ControlAfterGenerateStr) / sizeof(ControlAfterGenerateStr[0]); i++)
                {
                    list.push_back(std::string(ControlAfterGenerateStr[i]));
                }
                def = cast::to_i64(CAGRandomize);
            }

            combo_items_ptr.resize(list.size());
            combo_items_str.resize(list.size());
            def_state.value = int64_t(0);

            for (size_t idx = 0; idx < list.size(); idx++)
            {
                auto& item = list[idx];
                if (item == def)
                    def_state.value = cast::to_i64(idx);

                if (std::holds_alternative<std::string>(item))
                {
                    std::string* val = get_string(item);
                    combo_items_ptr[idx] = val->c_str();
                }
                else
                {
                    combo_items_str[idx] = any_to_string(item);
                    combo_items_ptr[idx] = combo_items_str[idx].c_str();
                }
            }
            def = def_state.value;
        }
    }

    if (!is_widget() && advanced)
    {
        LogInvalidJson("Non-widget pin can't be advanced");
        //LogAndThrowInternalError(ExcepCategory::Logic, "Non-widget pin can't be advanced");
    }

    def_state.value = def;
    def_state.w_name = ImGui::CalcTextSize(def_state.name.c_str()).x;

    if (input && is_widget())
    {
        switch (def_state.type)
        {
        case PinType::INT:
        case PinType::NUMBER:
        case PinType::FLOAT:
            w_value = ImGui::CalcTextSize("12345678901234567890").x + ImGui::GetStyle().ItemSpacing.x * 2 + (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2) * 2;
            break;
        case PinType::COLOR:
            w_value = (alpha_enabled ? 250.0f : 200.0f);
            break;
        case PinType::STRING:
            w_value = 300.0f;
            def_state.w_name = 0.0f;
            break;
        case PinType::COMBO:
            w_value = 150.0f;
            //for (auto& item : list)
            //{
            //    const auto w = ImGui::CalcTextSize(any_to_string(item).c_str()).x;
            //    w_value = std::max(w_value, w + 30);
            //    if (w_value < w + 30)
            //        w_value = w + 30;
            //}
            break;
        case PinType::BOOLEAN:
        default:
            w_value = 100.0f;
            break;
        }
    }

    // load options
    auto opt_adv = g::pOptsManager->pin_advanced_load(get_id());
    if (opt_adv.has_value())
        advanced = opt_adv.value();

    clamp_values();
}

template <typename T>
struct Clamp
{
    static void clamp(Pin& pin, primitive& p, T min, T max)
    {
        const auto old_val = std::get_if<T>(&p);
        const auto new_val = std::clamp(*old_val, min, max);
        if (*old_val != new_val)
        {
            LogWarnS("Pin '" << pin.def_state.name << ", value clump (" << old_val << ") != (" << new_val << ")");
        }
        p = new_val;
    }

    static void clamp(Pin& p, T min, T max)
    {
        clamp(p, p.def_state.value, min, max);
        clamp(p, p.min, min, max);
        clamp(p, p.max, min, max);
        clamp(p, p.def, min, max);
        clamp(p, p.step, min, max);
    }
};

void Pin::clamp_values()
{
    switch (def_state.type)
    {
    case PinType::INT:
    case PinType::NUMBER:
        if (is_unsigned)
            Clamp<uint64_t>::clamp(*this, 0, ULLONG_MAX / 2);
        else
            Clamp<int64_t>::clamp(*this, LLONG_MIN / 2, LLONG_MAX / 2);
        break;
    case PinType::FLOAT:
        Clamp<double>::clamp(*this, -DBL_MAX / 2.0, DBL_MAX / 2.0);
        break;
    }
}

void Pin::init_defaults()
{
    if (PinType::FLOAT == def_state.type)
    {
        def_state.value = double(0.0f);
        def = double(0.0f);
        min = double(FLT_MIN);
        max = double(FLT_MAX);
        step = double(0.0);
    }
    else if (PinType::INT == def_state.type || PinType::NUMBER == def_state.type)
    {
        def_state.value = int64_t(0);
        def = int64_t(0);
        min = int64_t(LLONG_MIN);
        max = int64_t(LLONG_MAX);
        step = int64_t(0);
    }
    else if (PinType::COLOR == def_state.type)
    {
        def_state.value = uint64_t(IM_COL32_WHITE);
        def = uint64_t(IM_COL32_WHITE);
        min = uint64_t(IM_COL32_BLACK);
        max = uint64_t(IM_COL32_WHITE);
        step = uint64_t(0);
    }
    else if (PinType::BOOLEAN == def_state.type)
    {
        def_state.value = bool(false);
        def = bool(false);
        min = bool(false);
        max = bool(true);
        step = bool(true);
        step = bool(false);
    }
    else if (PinType::STRING == def_state.type)
    {
        def_state.value = std::string("");
        def = std::string("");
        min = std::string("");
        max = std::string("");
        step = std::string("");
        step = std::string("");
    }
}

void Pin::precision_to_string()
{
    if (!round.has_value())
        return;

    int32_t decimals = 2;
    if (round.value() > 0.0 && round.value() < 1.0)
    {
        decimals = static_cast<int32_t>(std::ceil(-std::log10(round.value())));
        if (decimals > 6)
            decimals = 6;
    }

    precision = fmt::format(FMT_STRING("%.{}f"), decimals);
    precision_named = fmt::format(FMT_STRING("{}: %.{}f"), def_state.name, decimals);
}

void Pin::generate_pin_color()
{
    if (0 == def_state.type_hash)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Hash wasn't generated. id=" << get_id());
    }

    switch (def_state.type)
    {
    case PinType::INT:                  def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Int);                 return;
    case PinType::FLOAT:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Float);               return;
    case PinType::TEXT_LIST:
    case PinType::TEXT_LIST_SIMPLE:
    case PinType::STRING:               def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_String);              return;
    case PinType::BOOLEAN:              def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Boolean);             return;
    case PinType::ANY:                  def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Any);                 return;
    case PinType::COMBO:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Combo);               return;
    case PinType::COLOR:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Color);               return;
    case PinType::NUMBER:               def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Number);              return;

    case PinType::FLOATS:               def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Floats);              return;
    case PinType::SEED:                 def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Seed);                return;
    case PinType::MODEL:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Model);               return;
    case PinType::LATENT:               def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Latent);              return;
    case PinType::VAE:                  def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Vae);                 return;
    case PinType::CLIP:                 def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Clip);                return;    
    case PinType::IMAGE_LIST:
    case PinType::IMAGE_LIST_SIMPLE:
    case PinType::IMAGE_BOUNDS:
    case PinType::IMAGE:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Image);               return;
    case PinType::MASK:                 def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Mask);                return;
    case PinType::PROMPT_LIST:
    case PinType::SIMPLE_PROMPT_LIST:
    case PinType::CONDITIONING:         def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Conditioning);        return;
    case PinType::CLIP_VISION:          def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_ClipVision);          return;
    case PinType::CONTROL_NET:          def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_ControlNet);          return;
    case PinType::AUDIO:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Audio);               return;
    case PinType::VIDEO:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Video);               return;
    case PinType::IPADAPTER:            def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_IPAdapter);           return;
    case PinType::INSIGHTFACE:          def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_InsightFace);         return;
    case PinType::CONTROL_NET_STACK:    def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_ControlNetStack);     return;
    case PinType::CLIP_VISION_OUTPUT:   def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_ClipVisionOutput);    return;
    case PinType::NOISE:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Noise);               return;
    case PinType::SIGMAS:               def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Sigmas);              return;
    case PinType::SAMPLER:              def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Sampler);             return;
    case PinType::PIPE_LINE:            def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_PipeLine);            return;
    case PinType::LORA_STACK:
    case PinType::LORA_LIST:            def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Lora);                return;
    case PinType::SEGS:                 def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Segs);                return;
    case PinType::BBOX:                 def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_BBox);                return;
    case PinType::BUS:                  def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Bus);                 return;
    }

    // golden ratio
    double hue = (def_state.type_hash * 0.618033988749895);
    hue = std::fmod(hue, 1.0);

    // fix hue (S) и brightness (V)
    const double s = 0.75f;
    const double v = 0.95f;

    // HSV -> RGB
    const int32_t h_i = cast::to_i32(hue * 6);
    const double f = hue * 6 - h_i;
    const double p = v * (1 - s);
    const double q = v * (1 - f * s);
    const double t = v * (1 - (1 - f) * s);

    double r = 0.0f;
    double g = 0.0f;
    double b = 0.0f;
    switch (h_i)
    {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }

    def_state.pin_color = IM_COL32(cast::to_u32(r * 255), cast::to_u32(g * 255), cast::to_u32(b * 255), 255);
}

std::string Pin::any_to_string(const primitive& item)
{
    std::string ret;
    std::visit([&ret](const auto& v)
    {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, std::monostate>)
        {
            // nothing
        }
        else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, double>)
        {
            ret = std::to_string(v);
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            ret = (v ? "true" : "false");
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            ret = v;
        }
        else
        {
            static_assert(sizeof(T) == 0, "Undefined primitive type in visitor");
        }
    }, item);

    return ret;
}

uint64_t Pin::colors_to_int(const ImVec4& colors)
{
    ImColor clr(colors);
    return ImU32(clr);
}

uint64_t Pin::color_to_val(const std::string& clr, bool& alpha_in_string)
{
    if (clr.empty())
        return IM_COL32_WHITE;

    if ((clr.size() == 7 || clr.size() == 9) && clr[0] == '#')
    {
        uint32_t r = 0, g = 0, b = 0, a = 255;
        if (clr.size() == 7)
        {
            alpha_in_string = false;
            sscanf_s(clr.c_str(), "#%02x%02x%02x", &r, &g, &b);
        }
        else
        {
            alpha_in_string = true;
            sscanf_s(clr.c_str(), "#%02x%02x%02x%02x", &r, &g, &b, &a);
        }
        return IM_COL32(r, g, b, a);
    }

    static const std::unordered_map<std::string, uint32_t> named_colors =
    {
        { "white",   IM_COL32(255, 255, 255, 255)},
        { "black",   IM_COL32(0,   0,   0,   255)},
        { "red",     IM_COL32(255, 0,   0,   255)},
        { "green",   IM_COL32(0,   255, 0,   255)},
        { "blue",    IM_COL32(0,   0,   255, 255)},
        { "yellow",  IM_COL32(255, 255, 0,   255)},
        { "cyan",    IM_COL32(0,   255, 255, 255)},
        { "magenta", IM_COL32(255, 0,   255, 255)},
        { "gray",    IM_COL32(128, 128, 128, 255)},
        { "grey",    IM_COL32(128, 128, 128, 255)},
        { "orange",  IM_COL32(255, 165, 0,   255)},
        { "purple",  IM_COL32(128, 0,   128, 255)}
    };

    const auto it = named_colors.find(clr);
    if (it != named_colors.end())
        return it->second;

    // if we didn't get it - return bright magenta
    return IM_COL32(255, 0, 255, 255);
}

PinShape Pin::get_shape(PinType type)
{
    switch (type)
    {
        // --- SPECIAL ---
    case PinType::ANY:
        return PinShape::SemiCircle;

        // --- PRIMITIVES & DATA ---
    case PinType::INT:
    case PinType::FLOAT:
    case PinType::FLOATS:
    case PinType::BOOLEAN:
    case PinType::STRING:
    case PinType::NUMBER:
    case PinType::SEED:
    case PinType::UNIQUE_ID:
    case PinType::COLOR:
    case PinType::COMBO:
    case PinType::TEXT_LIST:
    case PinType::TEXT_LIST_SIMPLE:
    case PinType::LIST:
    case PinType::DICT:
    case PinType::API_KEY_COMFY_ORG:
    case PinType::AUTH_TOKEN_COMFY_ORG:
    case PinType::BATCH_COORDINATES:
    case PinType::FLOAT_CURVE:
    case PinType::GEMINI_INPUT_FILES:
    case PinType::OPENAI_INPUT_FILES:
    case PinType::OPENAI_CHAT_CONFIG:
    case PinType::IPADAPTER_PARAMS:
    case PinType::EMBEDS:
    case PinType::CROP_DATA:
    case PinType::LOSS_MAP:
        return PinShape::Diamond;

        // --- VISUAL & SPATIAL ---
    case PinType::IMAGE:
    case PinType::IMAGE_LIST:
    case PinType::IMAGE_LIST_SIMPLE:
    case PinType::IMAGE_BOUNDS:
    case PinType::MASK:
    case PinType::VIDEO:
    case PinType::AUDIO:
    case PinType::PLAYLIST:
    case PinType::LOAD_3D:
    case PinType::LOAD_3D_ANIMATION:
    case PinType::POSE_KEYPOINT:
    case PinType::SEG_ELT:
    case PinType::SEG_ELT_CROP_REGION:
    case PinType::SEG_ELT_CONTROL_NET_WRAPPER:
    case PinType::SEGS:
    case PinType::BBOX:
        return PinShape::Square;

        // --- FLOW & CONTROL ---
    case PinType::CONDITIONING:
    case PinType::CONTROL_NET:
    case PinType::CONTROL_NET_STACK:
    case PinType::PROMPT_LIST:
    case PinType::SIMPLE_PROMPT_LIST:
    case PinType::GUIDER:
    case PinType::SCHEDULE:
    case PinType::SCHEDULER_FUNC:
    case PinType::REGIONAL_PROMPTS:
    case PinType::WEIGHTS_STRATEGY:
    case PinType::CAMERA_CONTROL:
    case PinType::LOAD3D_CAMERA:
    case PinType::SIGMAS:
        return PinShape::Triangle;

        // --- TOOLS & PIPES ---
    case PinType::LORA_LIST:
    case PinType::LORA_STACK:
    case PinType::IPADAPTER:
    case PinType::INSIGHTFACE:
    case PinType::DETAILER_HOOK:
    case PinType::PK_HOOK:
    case PinType::HOOKS:
    case PinType::HOOK_KEYFRAMES:
    case PinType::UPSCALER_HOOK:
    case PinType::DETAILER_PIPE:
    case PinType::RGTHREE_CONTEXT:
    case PinType::UPSCALE_STACK:
    case PinType::GLIGEN:
    case PinType::PHOTOMAKER:
    case PinType::LUMA_CONCEPTS:
    case PinType::LUMA_REF:
    case PinType::PIXVERSE_TEMPLATE:
    case PinType::PIPE_LINE:
    case PinType::BUS:
        return PinShape::Hexagon;
    }

    return PinShape::Circle;
}

uint64_t Pin::get_hash(std::string str)
{
    return cast::to_u64(tools::str::hash_string(str));
}

} // namespace data
