
#pragma once
#include "core/base/id.h"

// TODO remove commented/unused
// TODO: output_is_list ??
// TODO: ANY
// TODO: COLOR, color_to_val

namespace data
{

struct Node;

struct PinState final
{
    int32_t links = 0;              // number of pin links
    bool converted = false;         // input pin converted to widget

    ImU32 pin_color = IM_COL32(255, 255, 255, 255); // will change if ANY pin
    PinType type = PinType::_none;  // will change if ANY pin
    std::string type_str;           // will change if ANY pin

    std::string name;
    float w_name = 0.0f;

    Primitive value;                // current value, if widget
    iid cag = 0;                    // cag pin id if cag_require is true

    PinState() noexcept = default;
    ~PinState() noexcept = default;

    PinState(const PinState&) = delete;
    PinState& operator=(const PinState&) = delete;

    PinState(PinState&&) noexcept = default;
    PinState& operator=(PinState&&) noexcept = default;

    void copy_to(PinState& other) const
    {
        other.pin_color = pin_color;
        other.type = type;
        other.type_str = type_str;
        other.name = name;
        other.w_name = w_name;
        other.value = value;
        // links, converted, and cag are intentionally not copied
    }
};

struct Pin : public base::HashID
{
public:
    // ctor for input pin
    Pin(Node* node, const std::string& pin_name, bool is_required, bool is_optional, bool is_hidden);
    // ctor for ouput pin
    Pin(Node* node, int32_t pin_index);
    ~Pin() noexcept override;

    static hid make_hash(const Node* node, const std::string& pin_name, int32_t pin_index);
    static hid make_hash(const std::string& node_class, const std::string& pin_name, int32_t pin_index);
    static bool is_primitive_type(PinType type, bool incl_colors = false) noexcept;

    void copy_int(Pin* dest) const;
    void copy_float(Pin* dest) const;
    void copy_number(Pin* dest) const;
    void copy_string(Pin* dest) const;
    void copy_bool(Pin* dest) const;
    void copy_combo(Pin* dest) const;

    bool is_widget(const PinState& state) const noexcept;

    static std::string any_to_string(const Primitive& item);
    static PinType type_str_to_type(const std::string& type_str);

    void initialize(const std::string& type_as_string);
    void finalize();
    void finalize_int();
    void finalize_float();
    void finalize_number();
    void finalize_combo();
    void finalize_bool();
    void finalize_color();

    static int64_t color_to_val(const std::string& clr, bool& alpha_in_string) noexcept;
    static int64_t colors_to_int(const ImVec4& colors) noexcept;

    static PinShape get_shape(PinType type) noexcept;

public:
    const Node* parent_node = nullptr;

    bool input = false;
    bool output = false;
    int32_t out_index = -1;

    bool required = false;
    bool optional = false;
    bool hidden = false;

    bool output_is_list = false;
    bool force_input = false;
    bool def_input = false; // TODO
    bool lazy = false; // TODO
    bool advanced = false;      // is visible on node

    bool can_link = true;
    bool can_rename = true;
    bool input_can_be_widget = true;

    bool image_upload = false;
    bool audio_upload = false;
    bool file_upload = false;
    bool video_upload = false;
    std::string image_folder;

    float w_value = 0.0f;
    std::string tooltip;

    bool type_any = false;
    PinState def_state;

    // all
    Primitive def;

    // string
    bool multiline = false;
    std::string placeholder;
    bool dynamic_prompts = false;

    // boolean
    std::string label_on;
    std::string label_on_named;
    std::string label_off;
    std::string label_off_named;

    // int, double, combo
    Primitive min;
    Primitive max;

    // int, double
    Primitive step;
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
    std::vector<Primitive> combo_items;
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
    void generate_pin_color() noexcept;
    void init_defaults();
    void precision_to_string();
    void clamp_values() noexcept;
    const char* prefix_from_visibility(bool is_required, bool is_optional, bool is_hidden) const noexcept;
};

} // namespace data
