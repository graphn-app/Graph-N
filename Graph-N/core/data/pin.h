
#pragma once
#include "core/base/id.h"

// TODO remove commented/unused
// TODO: output_is_list ??
// TODO: ANY
// TODO: NUMBER
// TODO: COLOR, color_to_val

namespace data
{

enum class PinType		// primitives and predefined colors only
{
	INT,
	FLOAT,
	BOOLEAN,
	COMBO,
	STRING,

	ANY,	// TODO
	COLOR,	// TODO
	NUMBER,	// TODO
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

	_none,
	_first = INT,
	_last = _count - 1
};

enum class ControlAfterGenerate
{
	fixed, increment, decrement, randomize,
	_count
};

struct Pin final : public base::LocalID
{
public:
	Pin() = default;
	~Pin() override = default;

	bool can_link(const Pin* other) const;
	void link_to(std::shared_ptr<Pin> other);
	void unlink();
	std::shared_ptr<Pin> get_link() const { return link.lock(); }

	static inline bool get_is_null(const primitive& value) { return std::holds_alternative<std::monostate>(value); }
	static inline int64_t get_int(const primitive& value) { return std::get<int64_t>(value); }
	static inline double get_float(const primitive& value) { return std::get<double>(value); }
	static inline bool get_bool(const primitive& value) { return std::get<bool>(value); }
	static inline const std::string& get_string(const primitive& value) { return std::get<std::string>(value); }

	void initialize(const std::string& type_as_string);
	void finalize();

	static uint32_t color_to_val(const std::string& clr);

	bool input = false;
	bool output = false;

	bool output_is_list = false;
	bool force_input = false;
	bool def_input = false;
	bool advanced = false;
	bool lazy = false; // TODO

	bool image_upload = false;
	bool audio_upload = false;
	bool file_upload = false;
	bool video_upload = false;
	std::string image_folder;

	bool is_known_type = false;

	uint32_t pin_color = 0;
	size_t hash = 0;
	std::string name;
	std::string tooltip;

	PinType type = PinType::_none;
	bool type_any = false;
	std::string type_str;

	// all
	primitive def;
	primitive val;

	// string
	bool multiline = false;
	std::string placeholder;
	bool dynamic_prompts = false;

	// boolean
	std::string label_on;
	std::string label_off;

	// int, double
	primitive min;
	primitive max;
	primitive step;
	bool display_as_number = false;
	bool display_as_color = false;
	bool display_as_slider = false;
	bool control_after_generate = false;
	ControlAfterGenerate control_after_generate_value = ControlAfterGenerate::randomize;
	const char* control_after_generate_to_string() const;

	// double
	std::optional<double> round;
	std::optional<bool> do_round;

	// combo
	bool multiselect = false;
	std::vector<primitive> list;
	std::string to_string(const std::vector<primitive>::iterator& it) const;

private:
	std::weak_ptr<Pin> link;
	static const std::unordered_map<std::string, PinType> pin_type_map;

private:
	void generate_pin_color();
	void init_defaults();
};

} // namespace data