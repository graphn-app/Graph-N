
#include "core/data/pin.h"
#include "core/data/node.h"

namespace data
{

const std::unordered_map<std::string, PinType> Pin::pin_type_map =
{
    { "INT", PinType::INT },				{ "FLOAT", PinType::FLOAT },				{ "BOOLEAN", PinType::BOOLEAN },
    { "COMBO", PinType::COMBO },			{ "STRING", PinType::STRING },				{ "*", PinType::ANY },

    { "COLOR", PinType::COLOR },			{ "NUMBER", PinType::NUMBER },				{ "INSIGHTFACE", PinType::INSIGHTFACE },
    { "IMAGE", PinType::IMAGE },			{ "MODEL", PinType::MODEL },				{ "MASK", PinType::MASK },
    { "LATENT", PinType::LATENT },			{ "VAE", PinType::VAE },					{ "CONDITIONING", PinType::CONDITIONING },
    { "CLIP", PinType::CLIP },				{ "NOISE", PinType::NOISE },				{ "CONTROL_NET", PinType::CONTROL_NET },
    { "SIGMAS", PinType::SIGMAS },			{ "BUS", PinType::BUS },					{ "IPADAPTER", PinType::IPADAPTER },
    { "SAMPLER", PinType::SAMPLER },		{ "AUDIO", PinType::AUDIO },				{ "SEED", PinType::SEED },
    { "SEGS", PinType::SEGS },				{ "CLIP_VISION", PinType::CLIP_VISION },	{ "CONTROL_NET_STACK", PinType::CONTROL_NET_STACK },
    { "PIPE_LINE", PinType::PIPE_LINE },	{ "VIDEO", PinType::VIDEO },				{ "LORA_STACK", PinType::LORA_STACK },
    { "BBOX", PinType::BBOX },				{ "LORA_LIST", PinType::LORA_LIST },		{ "CLIP_VISION_OUTPUT", PinType::CLIP_VISION_OUTPUT },
    { "FLOATS", PinType::FLOATS },
};

Pin::Pin(const Node* node, const std::string& pin_name)
    : base::HashID((node->node_class.empty() ? node->name : node->node_class) + "_IN_" + pin_name)
    , parent_node(node)
{
}

Pin::Pin(const Node* node, size_t pin_index)
    : base::HashID((node->node_class.empty() ? node->name : node->node_class) + "_OUT_" + std::to_string(pin_index))
    , parent_node(node)
{
}

Pin::~Pin() = default;

bool Pin::is_widget() const
{
    return !force_input &&
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
        is_known_type = true;
    }
    else
    {
        is_known_type = false;
        def_state.type = PinType::CUSTOM;
    }

    hash = tools::str::hash_string(def_state.type_str);

    init_defaults();
    generate_pin_color();
}

void Pin::finalize()
{
    if (display_as_color && display_as_slider)
    {
        display_as_color = display_as_slider = false;
        LogInvalidJson("Both display_as_color and display_as_slider can't be set");
        LogAndThrowInternalError(ExcepCategory::logic, "Both display_as_color and display_as_slider can't be set");
    }
    display_as_number = !(display_as_color || display_as_slider);

    if (input == output)
    {
        LogAndThrowInternalError(ExcepCategory::logic, "Pin can't be input and output at the same time");
    }

    if (!required && !optional && !hidden)
    {
        LogInvalidJson("Unknown pin visibility");
        LogAndThrowInternalError(ExcepCategory::logic, "Unknown pin visibility");
    }

    if (output && def_state.advanced)
    {
        LogInvalidJson("Output pin can't be advanced");
        LogAndThrowInternalError(ExcepCategory::logic, "Output pin can't be advanced");
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

    else if (PinType::INT == def_state.type)
    {
        if (!std::holds_alternative<int64_t>(step) || 0 == get_int(step))
            step = int64_t(1);

        precision = fmt::format(FMT_COMPILE("%lld"));
        precision_named = fmt::format(FMT_COMPILE("{}: %lld"), name);

        const bool seed = (tools::str::to_lower_ascii(name).find("seed") != std::string::npos);
        if (seed)
        {
            control_after_generate = true;
            def_state.control_after_generate_value = ControlAfterGenerate::randomize;
        }

        if (seed || max < min)  // usually max = -1 and min = 0 -> unsigned int
        {
            is_unsigned = true;
            precision = fmt::format(FMT_COMPILE("%llu"));
            precision_named = fmt::format(FMT_COMPILE("{}: %llu"), name);
            step = uint64_t(get_int(step));
            def = uint64_t(get_int(def));
            min = uint64_t(get_int(min));
            max = uint64_t(ULLONG_MAX);
        }
    }

    else if (PinType::BOOLEAN == def_state.type)
    {
        label_on_named = fmt::format(FMT_COMPILE("{}: {}"), name, (label_on.empty() ? "on" : label_on));
        label_off_named = fmt::format(FMT_COMPILE("{}: {}"), name, (label_off.empty() ? "off" : label_off));
    }

    else if (PinType::COLOR == def_state.type && std::holds_alternative<std::string>(def))
    {
        const ImU32 clr_im = color_to_val(std::get<std::string>(def), alpha_enabled);
        def = int64_t(clr_im);
        const ImVec4 clr_vec = ImColor(clr_im);
        def_state.colors[0] = clr_vec.x;
        def_state.colors[1] = clr_vec.y;
        def_state.colors[2] = clr_vec.z;
        def_state.colors[3] = clr_vec.w;
    }

    else if (PinType::COMBO == def_state.type)
    {
        combo_items_ptr.resize(list.size());
        combo_items_str.resize(list.size());
        
        for (size_t idx = 0; idx < list.size(); idx++)
        {
            auto& item = list[idx];
            if (item == def)
                def_state.combo_index = cast::to_i32(idx);

            if (std::holds_alternative<std::string>(item))
            {
                std::string& val = data::Pin::get_string(item);
                combo_items_ptr[idx] = val.c_str();
            }
            else
            {
                combo_items_str[idx] = primitive_to_string(item);
                combo_items_ptr[idx] = combo_items_str[idx].c_str();
            }
        }
    }

    def_state.value = def;
}

void Pin::init_defaults()
{
    if (PinType::FLOAT == def_state.type)
    {
        def_state.value = double(0.0f);
        def = double(0.0f);
        min = double(FLT_MIN);
        max = double(FLT_MAX);
    }
    else if (PinType::INT == def_state.type)
    {
        def_state.value = int64_t(0);
        def = int64_t(0);
        min = int64_t(LLONG_MIN);
        max = int64_t(LLONG_MAX);
    }
    else if (PinType::COLOR == def_state.type)
    {
        def_state.value = int64_t(IM_COL32_WHITE);
        def = int64_t(IM_COL32_WHITE);
        min = int64_t(IM_COL32(0, 0, 0, 255));
        max = int64_t(IM_COL32(255, 255, 255, 255));
    }
    else if (PinType::BOOLEAN == def_state.type)
    {
        def_state.value = bool(false);
        def = bool(false);
    }
    else if (PinType::STRING == def_state.type)
    {
        def_state.value = std::string("");
        def = std::string("");
    }
}

void Pin::precision_to_string()
{
    if (!round.has_value())
        return;

    int32_t decimals = 2;
    if (round.value() > 0.0 && round.value() < 1.0)
    {
        decimals = static_cast<int>(std::ceil(-std::log10(round.value())));
        if (decimals > 6)
            decimals = 6;
    }

    precision = fmt::format(FMT_COMPILE("%.{}f"), decimals);
    precision_named = fmt::format(FMT_COMPILE("{}: %.{}f"), name, decimals);
}

void Pin::generate_pin_color()
{
    if (0 == hash)
    {
        LogAndThrowInternalError(ExcepCategory::logic, "Hash wasn't generated. id=" << get_id());
    }

    if (is_known_type)	// hardcoded colors for some most used types
    {
        switch (def_state.type)
        {
            // --- Primitives ---
        case PinType::INT:						def_state.pin_color = IM_COL32(40, 200, 40, 255);   break;
        case PinType::FLOAT:					def_state.pin_color = IM_COL32(40, 200, 40, 255);   break;
        case PinType::STRING:					def_state.pin_color = IM_COL32(200, 200, 200, 255); break;
        case PinType::BOOLEAN:					def_state.pin_color = IM_COL32(180, 180, 180, 255); break;
        case PinType::ANY:						def_state.pin_color = IM_COL32_WHITE; break;
        case PinType::COMBO:					def_state.pin_color = IM_COL32(200, 200, 200, 255);   break;
        case PinType::COLOR:					def_state.pin_color = IM_COL32(255, 235, 59, 255);    break;
        case PinType::NUMBER:					def_state.pin_color = IM_COL32(46, 204, 113, 255);    break;

        case PinType::FLOATS:					def_state.pin_color = IM_COL32(26, 188, 156, 255);    break;
        case PinType::SEED:						def_state.pin_color = IM_COL32(52, 152, 219, 255);    break;
        case PinType::MODEL:					def_state.pin_color = IM_COL32(111, 108, 155, 255); break;
        case PinType::LATENT:					def_state.pin_color = IM_COL32(176, 83, 112, 255);  break;
        case PinType::VAE:						def_state.pin_color = IM_COL32(166, 46, 46, 255);   break;
        case PinType::CLIP:						def_state.pin_color = IM_COL32(227, 184, 54, 255);  break;
        case PinType::IMAGE:					def_state.pin_color = IM_COL32(91, 135, 163, 255);  break;
        case PinType::MASK:						def_state.pin_color = IM_COL32(93, 158, 89, 255);   break;
        case PinType::CONDITIONING:				def_state.pin_color = IM_COL32(195, 134, 52, 255);  break;
        case PinType::CLIP_VISION:				def_state.pin_color = IM_COL32(100, 100, 100, 255); break;
        case PinType::CONTROL_NET:				def_state.pin_color = IM_COL32(90, 160, 140, 255);  break;
        case PinType::AUDIO:					def_state.pin_color = IM_COL32(255, 160, 0, 255);     break;
        case PinType::VIDEO:					def_state.pin_color = IM_COL32(41, 128, 185, 255);    break;
        case PinType::IPADAPTER:				def_state.pin_color = IM_COL32(215, 189, 226, 255);   break;
        case PinType::INSIGHTFACE:				def_state.pin_color = IM_COL32(231, 76, 60, 255);     break;
        case PinType::CONTROL_NET_STACK:		def_state.pin_color = IM_COL32(39, 174, 96, 255);     break;
        case PinType::CLIP_VISION_OUTPUT:		def_state.pin_color = IM_COL32(241, 196, 15, 255);    break;
        case PinType::NOISE:					def_state.pin_color = IM_COL32(149, 165, 166, 255);   break;
        case PinType::SIGMAS:					def_state.pin_color = IM_COL32(140, 100, 80, 255);    break;
        case PinType::SAMPLER:					def_state.pin_color = IM_COL32(230, 126, 34, 255);    break;
        case PinType::PIPE_LINE:				def_state.pin_color = IM_COL32(255, 215, 0, 255);     break;
        case PinType::LORA_STACK:				def_state.pin_color = IM_COL32(255, 105, 180, 255);   break;
        case PinType::LORA_LIST:				def_state.pin_color = IM_COL32(219, 112, 147, 255);   break;
        case PinType::SEGS:						def_state.pin_color = IM_COL32(0, 255, 255, 255);     break;
        case PinType::BBOX:						def_state.pin_color = IM_COL32(127, 255, 0, 255);     break;
        case PinType::BUS:						def_state.pin_color = IM_COL32(255, 215, 0, 255);     break;

        default:
            {
                LogAndThrowInternalError(ExcepCategory::logic, "Some predefined colors were not set");
            }
        }

        return;
    }

    // golden ratio
    double hue = (hash * 0.618033988749895);
    hue = std::fmod(hue, 1.0);

    // fix hue (S) и brightness (V)
    const double s = 0.75f;
    const double v = 0.95f;

    // HSV -> RGB
    const int h_i = cast::to_i32(hue * 6);
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

std::string Pin::primitive_to_string(primitive& item)
{
    if (std::holds_alternative<std::monostate>(item))
        return "";

    if (std::holds_alternative<int64_t>(item))
        return std::to_string(Pin::get_int(item));

    if (std::holds_alternative<double>(item))
        return std::to_string(Pin::get_float(item));

    if (std::holds_alternative<bool>(item))
        return (Pin::get_bool(item) ? "true" : "false");

    return Pin::get_string(item);
}

int64_t Pin::colors_to_int(const float colors[4])
{
    ImVec4 vec(colors[0], colors[1], colors[2], colors[3]);
    ImColor clr(vec);
    return ImU32(clr);
}

uint32_t Pin::color_to_val(const std::string& clr, bool& alpha_in_string)
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

} // namespace data
