
#include "core/data/pin.h"

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

inline constexpr const char* ControlAfterGenerateStr[cast::to_szt(ControlAfterGenerate::_count)] =
{
	"fixed", "increment", "decrement", "randomize"
};

Pin::Pin(const std::string& node_class, const std::string& pin_name)
    : base::HashID(node_class + "_IN_" + pin_name)
{
}

Pin::Pin(const std::string& node_class, size_t pin_index)
    : base::HashID(node_class + "_OUT_" + std::to_string(pin_index))
{
}

Pin::~Pin() = default;

bool Pin::can_link(const Pin* other) const
{
	if (input == other->input || output == other->output)
		return false;
	if (type_any && other->type_any)
		return false;
	if (type_any || other->type_any)
		return true;

	if (PinType::COLOR == type && PinType::STRING == other->type ||
		PinType::COLOR == other->type && PinType::STRING == type)
	{
		return true;
	}

	return (
		type == other->type
		&&
		hash == other->hash
	);
}

void Pin::link_to(Pin* other)
{
	if (!can_link(other))
	{
		LogAndThrowInternalErr("Check can link before entablish connection");
	}

	if (type_any)
	{
		type = other->type;
		type_str = other->type_str;
		pin_color = other->pin_color;
	}
	else if (other->type_any)
	{
		other->type = type;
		other->type_str = type_str;
		other->pin_color = pin_color;
	}
	link = other;
}

void Pin::unlink()
{
	if (type_any)
	{
		type = PinType::ANY;
		type_str = "*";
		pin_color = IM_COL32_WHITE;
	}
	else if (link && link->type_any)
	{
        link->type = PinType::ANY;;
        link->type_str = "*";
        link->pin_color = IM_COL32_WHITE;
	}

    link = nullptr;
}

void Pin::initialize(const std::string& type_as_string)
{
	type_str = type_as_string;
	type = PinType::_none;
	auto map_it = pin_type_map.find(type_as_string);
	if (map_it != pin_type_map.end())
	{
		type = map_it->second;
		type_any = (PinType::ANY == type);
		is_known_type = true;			
	}
	else
	{
		is_known_type = false;
		type = PinType::CUSTOM;
	}

	hash = tools::str::hash_string(type_str);

	init_defaults();
	generate_pin_color();
}

void Pin::finalize()
{
	if (display_as_color && display_as_slider)
	{
		display_as_color = display_as_slider = false;
		LogInvalidJson("Both display_as_color and display_as_slider can't be set");
		LogAndThrowInternalErr("Both display_as_color and display_as_slider can't be set");
	}

	display_as_number = !(display_as_color || display_as_slider);

	if (input == output)
	{
		LogAndThrowInternalErr("Pin can't be input and output at the same time");
	}

	if (PinType::FLOAT == type)
	{
		if (!std::holds_alternative<double>(step))
		{
			if (round.has_value())
				step = round.value();
			else
				step = double(0.1f);
		}

		if (!do_round.has_value())
		{
			do_round = true;
		}

		if (!round.has_value())
		{
			if (do_round.value())
				round = 1.0f;
			else
				round = 0.f;
		}
	}

	else if (PinType::INT == type && (!std::holds_alternative<int64_t>(step) || 0 == get_int(step)))
	{
		step = int64_t(1);
	}

	else if (PinType::COLOR == type && std::holds_alternative<std::string>(def))
	{
		def = int64_t(color_to_val(std::get<std::string>(def)));
	}
}

void Pin::init_defaults()
{
	if (PinType::FLOAT == type)
	{
		val = double(0.0f);
		def = double(0.0f);
		min = double(FLT_MIN);
		max = double(FLT_MAX);
	}
	else if (PinType::INT == type)
	{
		val = int64_t(0);
		def = int64_t(0);
		min = int64_t(LLONG_MIN);
		max = int64_t(LLONG_MAX);
	}
	else if (PinType::COLOR == type)
	{
		val = int64_t(IM_COL32_WHITE);
		def = int64_t(IM_COL32_WHITE);
		min = int64_t(IM_COL32(0, 0, 0, 255));
		max = int64_t(IM_COL32(255, 255, 255, 255));
	}
	else if (PinType::BOOLEAN == type)
	{
		val = bool(false);
		def = bool(false);
	}
	else if (PinType::STRING == type)
	{
		val = "";
		def = "";
	}
}

const char* Pin::control_after_generate_to_string() const
{
	return ControlAfterGenerateStr[cast::to_szt(control_after_generate_value)];
}

void Pin::generate_pin_color()
{
	if (0 == hash)
	{
		LogAndThrowInternalErr("Hash wasn't generated. id=" << get_id());
	}

	if (is_known_type)	// hardcoded colors for some most used types
	{
        switch (type)
        {
            // --- Primitives ---
        case PinType::INT:						pin_color = IM_COL32(40, 200, 40, 255);   break;
        case PinType::FLOAT:					pin_color = IM_COL32(40, 200, 40, 255);   break;
        case PinType::STRING:					pin_color = IM_COL32(200, 200, 200, 255); break;
        case PinType::BOOLEAN:					pin_color = IM_COL32(180, 180, 180, 255); break;
        case PinType::ANY:						pin_color = IM_COL32_WHITE; break;

            // --- Core ComfyUI Types ---
        case PinType::MODEL:					pin_color = IM_COL32(111, 108, 155, 255); break;
        case PinType::LATENT:					pin_color = IM_COL32(176, 83, 112, 255);  break;
        case PinType::VAE:						pin_color = IM_COL32(166, 46, 46, 255);   break;
        case PinType::CLIP:						pin_color = IM_COL32(227, 184, 54, 255);  break;
        case PinType::IMAGE:					pin_color = IM_COL32(91, 135, 163, 255);  break;
        case PinType::MASK:						pin_color = IM_COL32(93, 158, 89, 255);   break;
        case PinType::CONDITIONING:				pin_color = IM_COL32(195, 134, 52, 255);  break;
        case PinType::CLIP_VISION:				pin_color = IM_COL32(100, 100, 100, 255); break;
        case PinType::CONTROL_NET:				pin_color = IM_COL32(90, 160, 140, 255);  break;

            // --- Extended Basic Types ---
        case PinType::COMBO:					pin_color = IM_COL32(200, 200, 200, 255);   break; // Grey (Generic)
        case PinType::COLOR:					pin_color = IM_COL32(255, 235, 59, 255);    break; // Bright Yellow
        case PinType::NUMBER:					pin_color = IM_COL32(46, 204, 113, 255);    break; // Green (Int/Num standard)
        case PinType::FLOATS:					pin_color = IM_COL32(26, 188, 156, 255);    break; // Turquoise (Distinct from Int)
        case PinType::SEED:						pin_color = IM_COL32(52, 152, 219, 255);    break; // Light Blue (Seed)

        // --- Media ---
        case PinType::AUDIO:					pin_color = IM_COL32(255, 160, 0, 255);     break; // Orange (Waveforms)
        case PinType::VIDEO:					pin_color = IM_COL32(41, 128, 185, 255);    break; // Dark Blue (Video frames)

        // --- Models & Adapters ---
        case PinType::IPADAPTER:				pin_color = IM_COL32(215, 189, 226, 255);   break; // Light Purple
        case PinType::INSIGHTFACE:				pin_color = IM_COL32(231, 76, 60, 255);     break; // Red (FaceID)
        case PinType::CONTROL_NET_STACK:		pin_color = IM_COL32(39, 174, 96, 255);     break; // Dark Green (ControlNet)
        case PinType::CLIP_VISION_OUTPUT:		pin_color = IM_COL32(241, 196, 15, 255);    break; // Gold (CLIP related)

        // --- Sampling & Noise ---
        case PinType::NOISE:					pin_color = IM_COL32(149, 165, 166, 255);   break; // Grey/Concrete (Noise)
        case PinType::SIGMAS:					pin_color = IM_COL32(140, 100, 80, 255);    break; // Brown (Math/Technical)
        case PinType::SAMPLER:					pin_color = IM_COL32(230, 126, 34, 255);    break; // Carrot Orange

        // --- Stacks & Bundles ---
        case PinType::PIPE_LINE:				pin_color = IM_COL32(255, 215, 0, 255);     break; // Gold (Bus/Bundle)
        case PinType::LORA_STACK:				pin_color = IM_COL32(255, 105, 180, 255);   break; // Pink (LoRA standard)
        case PinType::LORA_LIST:				pin_color = IM_COL32(219, 112, 147, 255);   break; // Pale Pink

        // --- Vision / Geometry ---
        case PinType::SEGS:						pin_color = IM_COL32(0, 255, 255, 255);     break; // Cyan (Segmentation)
        case PinType::BBOX:						pin_color = IM_COL32(127, 255, 0, 255);     break; // Lime (Geometry)
        
        default:
			{
				LogAndThrowInternalErr("Some predefined colors were not set");
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

	pin_color = IM_COL32(cast::to_u32(r * 255), cast::to_u32(g * 255), cast::to_u32(b * 255), 255);
}

std::string to_string(const std::vector<primitive>::iterator& it)
{
	const auto& value = *it;

	if (std::holds_alternative<std::monostate>(value))
		return "";

	if (std::holds_alternative<int64_t>(value))
		return std::to_string(Pin::get_int(value));

	if (std::holds_alternative<double>(value))
		return std::to_string(Pin::get_float(value));

	if (std::holds_alternative<bool>(value))
		return (Pin::get_bool(value) ? "true" : "false");

	return Pin::get_string(value);
}

uint32_t Pin::color_to_val(const std::string& clr)
{
	if (clr.empty())
		return IM_COL32_WHITE;

	if ((clr.size() == 7 || clr.size() == 9) && clr[0] == '#')
	{
		uint32_t r = 0, g = 0, b = 0, a = 255;
		if (clr.size() == 7)
			sscanf_s(clr.c_str(), "#%02x%02x%02x", &r, &g, &b);
		else
			sscanf_s(clr.c_str(), "#%02x%02x%02x%02x", &r, &g, &b, &a);
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