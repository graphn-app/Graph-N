#include "core/workflow/pin.h"
#include "core/workflow/node.h"
#include "core/managers/settings_manager.h"
#include "imgui_widget_limits.h"

namespace data
{

const std::unordered_map<std::string, PinType> Pin::pin_type_map =
{
    { "INT", PinType::INT },				            { "FLOAT", PinType::FLOAT },			            { "NUMBER", PinType::NUMBER },
    { "BOOLEAN", PinType::BOOLEAN },                    { "COMBO", PinType::COMBO },			            { "STRING", PinType::STRING },
    { "*", PinType::ANY },                              { "COLOR", PinType::COLOR },			            { "INSIGHTFACE", PinType::INSIGHTFACE },
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
    { "UPSCALE_STACK", PinType::UPSCALE_STACK },        { "GLIGEN", PinType::GLIGEN },		                { "OPENAI_CHAT_CONFIG", PinType::OPENAI_CHAT_CONFIG },

    { REROUTE_SPLITTED_PIN, PinType::REROUTE_SPLITTED }
};

Pin::Pin(Node* node, const std::string& pin_name, bool is_required, bool is_optional/*, bool is_hidden*/)
    : base::HashID(node->node_class + "_IN_" + pin_name)
    , parent_node(node)
{
    flags.set(PinObjFlags::Input, PinObjFlags::CanLink, PinObjFlags::CanUnlink, PinObjFlags::CanRename, PinObjFlags::InputCanBeWidget, PinObjFlags::ClrAlphaEnabled);
    flags.add_remove_if(PinObjFlags::Required, is_required);
    flags.add_remove_if(PinObjFlags::Optional, is_optional);
    node->flags.add(NodeObjFlags::HasInput);
}

Pin::Pin(Node* node, int32_t pin_index)
    : base::HashID(node->node_class + "_OUT_" + std::to_string(pin_index))
    , parent_node(node)
    , out_index(pin_index)
{
    flags.set(PinObjFlags::Output, PinObjFlags::CanLink, PinObjFlags::CanUnlink, PinObjFlags::CanRename);
    node->flags.add(NodeObjFlags::HasOutput);
}

Pin::~Pin() noexcept = default;

hid Pin::make_hash(const Node* node, const std::string& pin_name, int32_t pin_index)
{
    return make_hash(node->node_class, pin_name, pin_index);
}

hid Pin::make_hash(const std::string& node_class, const std::string& pin_name, int32_t pin_index)
{
    if (-1 == pin_index)
        return base::HashID::make_hash(node_class + "_IN_" + pin_name);
    return base::HashID::make_hash(node_class + "_OUT_" + std::to_string(pin_index));
}

bool Pin::is_primitive_type(PinType type, bool incl_colors /*= false*/) noexcept
{
    if (incl_colors && PinType::COLOR == type)
        return true;
    return
        PinType::INT == type || PinType::FLOAT == type || PinType::NUMBER == type ||
        PinType::COMBO == type || PinType::STRING == type || PinType::BOOLEAN == type
    ;
}

void Pin::copy_int(Pin* dest) const
{
    LogRAssert(dest, "Zero ptr", );
    dest->def = def;
    dest->def_state.min = def_state.min;
    dest->def_state.max = def_state.max;
    dest->def_state.step = def_state.step;
    dest->w_value = ImGui::CalcTextSize("12345678901234567890").x + ImGui::GetStyle().ItemSpacing.x * 2 + (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2) * 2;
    dest->tooltip = tooltip;
    def_state.copy_to(dest->def_state);
    dest->finalize_int();
}

void Pin::copy_float(Pin* dest) const
{
    LogRAssert(dest, "Zero ptr", );
    dest->def = def;
    dest->def_state.min = def_state.min;
    dest->def_state.max = def_state.max;
    dest->def_state.step = def_state.step;
    dest->w_value = ImGui::CalcTextSize("12345678901234567890").x + ImGui::GetStyle().ItemSpacing.x * 2 + (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2) * 2;
    dest->tooltip = tooltip;
    dest->round = round;
    dest->do_round = do_round;
    dest->precision = precision;
    dest->precision_named = precision_named;
    dest->finalize_float();
}

void Pin::copy_number(Pin* dest) const
{
    static_assert(std::same_as<number_t, double>, "Check this function if number_t changes");

    LogRAssert(dest, "Zero ptr", );
    dest->def = def;
    dest->def_state.min = def_state.min;
    dest->def_state.max = def_state.max;
    dest->def_state.step = def_state.step;
    dest->w_value = ImGui::CalcTextSize("12345678901234567890").x + ImGui::GetStyle().ItemSpacing.x * 2 + (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2) * 2;
    dest->tooltip = tooltip;
    dest->round = round;
    dest->do_round = do_round;
    dest->precision = precision;
    dest->precision_named = precision_named;
    dest->finalize_number();
}

void Pin::copy_string(Pin* dest) const
{
    LogRAssert(dest, "Zero ptr", );
    dest->def = def;
    dest->w_value = 200.0f;
    dest->tooltip = tooltip;
    dest->flags.add_remove_if(PinObjFlags::Multiline, flags.has(PinObjFlags::Multiline));
    dest->placeholder = placeholder;
    def_state.copy_to(dest->def_state);
}

void Pin::copy_bool(Pin* dest) const
{
    LogRAssert(dest, "Zero ptr", );
    dest->def = def;
    dest->def_state.min = def_state.min;
    dest->def_state.max = def_state.max;
    dest->def_state.step = def_state.step;
    dest->w_value = 100.0f;
    dest->tooltip = tooltip;
    dest->label_on = label_on;
    dest->label_on_named = label_on_named;
    dest->label_off = label_off;
    dest->label_off_named = label_off_named;
    dest->finalize_bool();
}

void Pin::copy_combo(Pin* dest) const
{
    LogRAssert(dest, "Zero ptr", );
    dest->def = def;
    dest->def_state.min = def_state.min;
    dest->def_state.max = def_state.max;
    dest->def_state.step = def_state.step;
    dest->w_value = 150.0f;
    dest->tooltip = tooltip;
    dest->flags.add_remove_if(PinObjFlags::Multiselect, flags.has(PinObjFlags::Multiselect));
    dest->combo_items = combo_items;
    dest->finalize_combo();
}

bool Pin::is_widget() const noexcept
{
    return flags.has(PinObjFlags::InputCanBeWidget) && flags.has_none(PinObjFlags::Output, PinObjFlags::ForceInput) &&
        cast::to_i64(def_state.type) >= cast::to_i64(PinType::_first_widget) && cast::to_i64(def_state.type) <= cast::to_i64(PinType::_last_widget);
}

PinType Pin::type_str_to_type(const std::string& type_str)
{
    auto map_it = pin_type_map.find(type_str);
    if (map_it != pin_type_map.end())
        return map_it->second;
    return PinType::CUSTOM;
}

void Pin::initialize(const std::string& type_as_string)
{
    def_state.type_str = type_as_string;
    def_state.type = type_str_to_type(type_as_string);
    flags.add_remove_if(PinObjFlags::TypeAny, (PinType::ANY == def_state.type));

    init_defaults();
    generate_pin_color();
}

void Pin::finalize()
{
    const auto input = flags.has(PinObjFlags::Input);
    const auto output = flags.has(PinObjFlags::Output);
    const auto adv = flags.has(PinObjFlags::Advanced);
    
    LogAssert(input != output, "Pin can't be input and output at the same time");
    LogAssert(!(output && adv), "Output pin can't be advanced");

    if (input)
    {
        if (flags.has_all(PinObjFlags::DisplAsColor, PinObjFlags::DisplAsSlider))
        {
            flags.remove(PinObjFlags::DisplAsColor, PinObjFlags::DisplAsSlider);
            LogInvalidJson("Both display_as_color and display_as_slider can't be set");
            LogErrorS("Both display_as_color and display_as_slider can't be set");
        }
        flags.add_remove_if(PinObjFlags::DisplAsNumber, flags.has_none(PinObjFlags::DisplAsColor, PinObjFlags::DisplAsSlider));

        if (flags.has_none(PinObjFlags::Required, PinObjFlags::Optional))
        {
            flags.add(PinObjFlags::Required);
            LogInvalidJson("Unknown pin visibility");
            LogErrorS("Unknown pin visibility");
        }

        if (PinType::FLOAT == def_state.type)
            finalize_float();
        else if (PinType::NUMBER == def_state.type)
            finalize_number();
        else if (PinType::INT == def_state.type)
            finalize_int();
        else if (PinType::BOOLEAN == def_state.type)
            finalize_bool();
        else if (PinType::COLOR == def_state.type && def.is_string())
            finalize_color();
        else if (PinType::COMBO == def_state.type)
            finalize_combo();
    }

    //if (!is_widget(def_state) && advanced)
    //{
    //    LogInvalidJson("Non-widget pin can't be advanced");
    //    LogErrorS("Non-widget pin can't be advanced");
    //}

    def_state.value = def;
    def_state.w_name = ImGui::CalcTextSize(def_state.name.c_str()).x;

    if (input && is_widget())
    {
        switch (def_state.type)
        {
        case PinType::INT:
        case PinType::FLOAT:
        case PinType::NUMBER:
            w_value = ImGui::CalcTextSize("12345678901234567890").x + ImGui::GetStyle().ItemSpacing.x * 2 + (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2) * 2;
            break;
        case PinType::COLOR:
            w_value = (flags.has(PinObjFlags::ClrAlphaEnabled) ? 250.0f : 200.0f);
            break;
        case PinType::STRING:
            w_value = 200.0f;
            def_state.w_name = 0.0f;
            break;
        case PinType::COMBO:
            w_value = 150.0f;
            //for (auto& item : combo_items)
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

    clamp_values();
}

void Pin::finalize_int()
{
    precision = fmt::format(FMT_STRING("%lld"));
    precision_named = fmt::format(FMT_STRING("{}: %lld"), def_state.name);

    const bool seed = (tools::str::to_lower_ascii(def_state.name).find("seed") != std::string::npos);
    if (seed || def_state.max < def_state.min)  // usually max = -1 and min = 0 -> unsigned int
    {
        def_state.step = int64_t(1);
        def = int64_t(0);
        def_state.min = int64_t(IM_S64_MIN_WIDGET);
        def_state.max = int64_t(IM_S64_MAX_WIDGET);
    }
    else
    {
        if (0 == def_state.step.get_int())
            def_state.step = int64_t(1);
        def_state.min = std::max(def_state.min.get_int(), IM_S64_MIN_WIDGET);
        def_state.max = std::min(def_state.max.get_int(), IM_S64_MAX_WIDGET);
    }

    if (def_state.min.get_int() == def_state.max.get_int())
    {
        def_state.min = int64_t(IM_S64_MIN_WIDGET);
        def_state.max = int64_t(IM_S64_MAX_WIDGET);
    }
    if (def_state.step.get_int() == 0)
    {
        def_state.step = int64_t(1);
    }
}

void Pin::finalize_float()
{
    if (!def_state.step.is_float())
        def_state.step = round.value_or(0.01);
    if (def_state.step.get_float() == 0)
        def_state.step = double(0.01);

    if (def_state.max < def_state.min)
        std::swap(def_state.min, def_state.max);

    if (!def.is_float()) def = double(0.0);
    if (!def_state.min.is_float()) def_state.min = IM_DBL_MIN_WIDGET;
    if (!def_state.max.is_float()) def_state.max = IM_DBL_MAX_WIDGET;
    def_state.min = std::max(def_state.min.get_float(), IM_DBL_MIN_WIDGET);
    def_state.max = std::min(def_state.max.get_float(), IM_DBL_MAX_WIDGET);

    if (def_state.min.get_float() == def_state.max.get_float())
    {
        def_state.min = double(IM_DBL_MIN_WIDGET);
        def_state.max = double(IM_DBL_MAX_WIDGET);
    }
    if (def_state.step.get_float() == 0.0)
    {
        def_state.step = double(0.1);
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

void Pin::finalize_number()
{
    static_assert(std::same_as<number_t, double>, "Check this function if number_t changes");

    if (!def_state.step.is_number())
        def_state.step = number(round.value_or(NUMBER_STEP));

    if (def_state.max < def_state.min)
        std::swap(def_state.min, def_state.max);

    if (!def.is_number()) def = number(NUMBER_DEFAULT);
    if (!def_state.min.is_number()) def_state.min = number(IM_NUM_MIN_WIDGET);
    if (!def_state.max.is_number()) def_state.max = number(IM_NUM_MAX_WIDGET);
    def_state.min = std::max(def_state.min.get_number(), number(IM_NUM_MIN_WIDGET));
    def_state.max = std::min(def_state.max.get_number(), number(IM_NUM_MAX_WIDGET));

    if (def_state.min.get_number() == def_state.max.get_number())
    {
        def_state.min = number(IM_NUM_MIN_WIDGET);
        def_state.max = number(IM_NUM_MAX_WIDGET);
    }
    if (def_state.step.get_number() == number(NUMBER_ZERO))
    {
        def_state.step = number(NUMBER_STEP);
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

void Pin::finalize_combo()
{
    combo_items_ptr.clear();
    combo_items_str.clear();

    combo_items_ptr.resize(combo_items.size());
    combo_items_str.resize(combo_items.size());
    def_state.value = int64_t(0);

    def_state.min = int64_t(0);
    def_state.max = combo_items.empty() ? int64_t(0) : cast::to_i64(combo_items.size() - 1);

    for (size_t idx = 0; idx < combo_items.size(); idx++)
    {
        auto& item = combo_items[idx];
        if (item == def)
            def_state.value = cast::to_i64(idx);

        if (item.is_string())
        {
            const std::string& val = item.get_string();
            combo_items_ptr[idx] = val.c_str();
            combo_items_str[idx] = val;
        }
        else
        {
            combo_items_str[idx] = any_to_string(item);
            combo_items_ptr[idx] = combo_items_str[idx].c_str();
        }
    }

    def = def_state.value;
}

void Pin::finalize_bool()
{
    label_on_named = fmt::format(FMT_STRING("{}: {}"), def_state.name, (label_on.empty() ? "on" : label_on));
    label_off_named = fmt::format(FMT_STRING("{}: {}"), def_state.name, (label_off.empty() ? "off" : label_off));
}

void Pin::finalize_color()
{
    precision = fmt::format(FMT_STRING("%lld"));
    precision_named = fmt::format(FMT_STRING("{}: %lld"), def_state.name);

    if (def_state.max < def_state.min)
        std::swap(def_state.min, def_state.max);
}

namespace {
template <typename T>
void clamp_primitive(const Pin& pin, Primitive& p, T min_val, T max_val) noexcept
{
    const T* current_ptr = std::get_if<T>(&p.value());
    if (!current_ptr)
        return;

    const T old_val = *current_ptr;

    if (p.clamp<T>(min_val, max_val))
    {
        // Calculate new_val directly instead of re-fetching from variant
        const T new_val = std::clamp(old_val, min_val, max_val);
        LogErrorS("Pin '" << pin.def_state.name << "', value clamped (" << old_val << ") != (" << new_val << ")");
    }
}

template <typename T>
void clamp_primitive(Pin& p, T min, T max) noexcept
{
    clamp_primitive<T>(p, p.def_state.value, min, max);
    clamp_primitive<T>(p, p.def_state.min, min, max);
    clamp_primitive<T>(p, p.def_state.max, min, max);
    clamp_primitive<T>(p, p.def, min, max);
    clamp_primitive<T>(p, p.def_state.step, min, max);
}
} // namespace

void Pin::clamp_values() noexcept
{
    switch (def_state.type)
    {
    case PinType::INT:
        clamp_primitive<int64_t>(*this, IM_S64_MIN_WIDGET, IM_S64_MAX_WIDGET);
        break;
    case PinType::FLOAT:
        clamp_primitive<double>(*this, IM_DBL_MIN_WIDGET, IM_DBL_MAX_WIDGET);
        break;
    case PinType::NUMBER:
        clamp_primitive<number>(*this, IM_NUM_MIN_WIDGET, IM_NUM_MAX_WIDGET);
        break;
    }
}

void Pin::init_defaults()
{
    if (PinType::FLOAT == def_state.type)
    {
        def_state.value = double(0.0f);
        def = double(0.0f);
        def_state.min = double(IM_DBL_MIN_WIDGET);
        def_state.max = double(IM_DBL_MAX_WIDGET);
        def_state.step = double(0.1);
    }
    if (PinType::NUMBER == def_state.type)
    {
        def_state.value = number(NUMBER_DEFAULT);
        def = number(NUMBER_DEFAULT);
        def_state.min = number(IM_NUM_MIN_WIDGET);
        def_state.max = number(IM_NUM_MAX_WIDGET);
        def_state.step = number(NUMBER_STEP);
    }
    else if (PinType::INT == def_state.type)
    {
        def_state.value = int64_t(0);
        def = int64_t(0);
        def_state.min = int64_t(IM_S64_MIN_WIDGET);
        def_state.max = int64_t(IM_S64_MAX_WIDGET);
        def_state.step = int64_t(1);
    }
    else if (PinType::COLOR == def_state.type)
    {
        def_state.value = int64_t(IM_COL32_WHITE);
        def = int64_t(IM_COL32_WHITE);
        def_state.min = int64_t(IM_COL32_BLACK);
        def_state.max = int64_t(IM_COL32_WHITE);
        def_state.step = int64_t(1);
    }
    else if (PinType::BOOLEAN == def_state.type)
    {
        def_state.value = bool(false);
        def = bool(false);
        def_state.min = bool(false);
        def_state.max = bool(true);
        def_state.step = bool(true);
    }
    else if (PinType::STRING == def_state.type)
    {
        def_state.value = std::string("");
        def = std::string("");
        def_state.min = std::string("");
        def_state.max = std::string("");
        def_state.step = std::string("");
    }
}

void Pin::precision_to_string()
{
    if (!round.has_value())
        return;

    int32_t decimals = 2;
    if (round.value() > 0.0 && round.value() < 1.0)
    {
        decimals = cast::to_i32(std::ceil(-std::log10(round.value())));
        if (decimals > 6)
            decimals = 6;
    }

    precision = fmt::format(FMT_STRING("%.{}f"), decimals);
    precision_named = fmt::format(FMT_STRING("{}: %.{}f"), def_state.name, decimals);
}

void Pin::generate_pin_color() noexcept
{
    LogAssert(def_state.type != PinType::_none, "Pin type wasn't set. h:" << get_hash());

    switch (def_state.type)
    {
    case PinType::INT:                  def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Int);                 return;
    case PinType::FLOAT:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Float);               return;
    case PinType::NUMBER:               def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Float);               return;
    case PinType::TEXT_LIST:
    case PinType::TEXT_LIST_SIMPLE:
    case PinType::STRING:               def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_String);              return;
    case PinType::BOOLEAN:              def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Boolean);             return;
    case PinType::ANY:                  def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Any);                 return;
    case PinType::COMBO:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Combo);               return;
    case PinType::COLOR:                def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_Color);               return;

    case PinType::REROUTE_SPLITTED:     def_state.pin_color = g::pOptsManager->color_load(AppColor::Pin_RerouteSplitted);     return;

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
    double hue = cast::to_u64(tools::str::hash_stable(def_state.type_str)) * 0.618033988749895;
    hue = std::fmod(hue, 1.0);

    // fix hue (S) & brightness (V)
    constexpr double s = 0.75f;
    constexpr double v = 0.95f;

    // HSV -> RGB
    const int32_t h_i = cast::to_i32(hue * 6);
    const double f = hue * 6 - h_i;
    constexpr double p = v * (1 - s);
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

std::string Pin::any_to_string(const Primitive& item)
{
    std::string ret;
    std::visit([&ret](const auto& v)
    {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, std::monostate>)
        {
            // do nothing
        }
        else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, double>)
        {
            ret = std::to_string(v);
        }
        else if constexpr (std::is_same_v<T, number>)
        {
            static_assert(std::same_as<number_t, double>, "Check this function if number_t changes");
            ret = std::to_string(v.underlying_type());
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
    }, item.value());

    return ret;
}

int64_t Pin::colors_to_int(const ImVec4& colors) noexcept
{
    const ImColor clr(colors);
    return cast::to_i64(ImU32(clr));
}

int64_t Pin::color_to_val(const std::string& clr, bool& alpha_in_string) noexcept
{
    if (clr.empty())
        return cast::to_i64(IM_COL32_WHITE);

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
        return cast::to_i64(IM_COL32(r, g, b, a));
    }

    struct ColorEntry final
    {
        std::string_view name;
        ImU32 value = 0;
    };

    constexpr std::array<ColorEntry, 12> named_colors = // must be sorted A-Z
    {{
        { "black",   IM_COL32(0,   0,   0,   255) },
        { "blue",    IM_COL32(0,   0,   255, 255) },
        { "cyan",    IM_COL32(0,   255, 255, 255) },
        { "gray",    IM_COL32(128, 128, 128, 255) },
        { "green",   IM_COL32(0,   255, 0,   255) },
        { "grey",    IM_COL32(128, 128, 128, 255) },
        { "magenta", IM_COL32(255, 0,   255, 255) },
        { "orange",  IM_COL32(255, 165, 0,   255) },
        { "purple",  IM_COL32(128, 0,   128, 255) },
        { "red",     IM_COL32(255, 0,   0,   255) },
        { "white",   IM_COL32(255, 255, 255, 255) },
        { "yellow",  IM_COL32(255, 255, 0,   255) }
    }};
    
    const auto it = std::ranges::lower_bound(named_colors, clr, std::ranges::less{}, &ColorEntry::name);
    if (it != named_colors.end() && it->name == clr)
        return cast::to_i64(it->value);
    
    // if we didn't get it - return bright magenta
    return cast::to_i64(IM_COL32(255, 0, 255, 255));
}

PinShape Pin::get_shape(PinType type) noexcept
{
    LogAssertS(type != PinType::_none, "Undefined pin type");

    switch (type)
    {
        // --- SPECIAL ---
    case PinType::ANY:
    case PinType::REROUTE_SPLITTED:
        return PinShape::SemiCircle;

        // --- PRIMITIVES & DATA ---
    case PinType::INT:
    case PinType::FLOAT:
    case PinType::NUMBER:
    case PinType::FLOATS:
    case PinType::BOOLEAN:
    case PinType::STRING:
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

} // namespace data
