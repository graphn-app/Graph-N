/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

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

    ControlAfterGenerate cag_val = ControlAfterGenerate::Fixed;
    // int, double, combo
    Primitive min;
    Primitive max;
    // int, double
    Primitive step;

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
        other.cag_val = cag_val;
        other.min = min;
        other.max = max;
        other.step = step;
        // links and converted are intentionally not copied
    }
};

struct Pin : public base::HashID
{
public:
    // ctor for input pin
    Pin(Node* node, const std::string& pin_name, bool is_required, bool is_optional/*, bool is_hidden*/);
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

    bool is_widget() const noexcept;

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

    static bool color_to_val(const std::string& clr, int64_t& val, bool& alpha_in_string) noexcept;
    static int64_t colors_to_int(const ImVec4& colors) noexcept;

    static PinShape get_shape(PinType type) noexcept;

    static const char* cag_to_string(ControlAfterGenerate cag) noexcept;
    static ControlAfterGenerate string_to_cag(const char* cag) noexcept;

public:
    ObjectFlags<PinObjFlags> flags;

    int32_t out_index = -1;

    std::string image_folder;

    float w_value = 0.0f;
    std::string tooltip;

    PinState def_state;

    // all
    Primitive def; // needed?

    // string
    std::string placeholder;

    // boolean
    std::string label_on;
    std::string label_on_named;
    std::string label_off;
    std::string label_off_named;

    // double
    std::optional<double> round;
    std::optional<bool> do_round;
    std::string precision;
    std::string precision_named;

    // combo
    std::vector<Primitive> combo_items;
    std::vector<const char*> combo_items_ptr;
    std::vector<std::string> combo_items_str;

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
