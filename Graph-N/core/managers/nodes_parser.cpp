
#include "core/managers/nodes_parser.h"
#include "core/managers/models_manager.h"
#include "core/managers/settings_manager.h"
#include "core/data/pin.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

NodesParser::~NodesParser()
{
    LogF;
}

void NodesParser::check_status(size_t& parsed_item, size_t& parse_total)
{
    using namespace std::chrono_literals;
    if (!m_parse_finished && m_future.wait_for(0ms) == std::future_status::ready)
    {
        if (m_thread.joinable())
            m_thread.join();

        if (!m_stop_thread_flag)    // ignore if parse thread was stopped by user
        {
            auto copy = m_future.get();
            if (m_parse_failed)
            {
                Log("Parsed: fail");
            }
            else
            {
                Log("Parsed: ok");
                // this is UI thread, so it is safe to recreate on the fly
                g::pGraphN->invalidate_comfyui_objects();
                Log(APP_COMFYUI " listeners notified to invalidate data");
                g::pNodesManager->init_from_copy(*copy.get());
                Log(APP_COMFYUI " data copied");
                g::pGraphN->resolve_comfyui_objects();
                Log(APP_COMFYUI " listeners notified to resolve data");

                g::pFlow->set_nodes_ready();
            }
        }
        m_stop_thread_flag = false;
        m_parse_finished = true;
    }

    parsed_item = m_parsed_item;
    parse_total = m_parsed_items_total;
}

void NodesParser::parse(std::string&& data)
{
    Log("To parse: " << data.size() << " bytes")
    stop();
    m_parse_finished = false;
    m_parse_failed = false;
    m_stop_thread_flag = false;
    m_json_data = std::move(data);

    m_parsed_item = 0;
    m_parsed_items_total = 0;

    std::packaged_task<std::shared_ptr<NodesManager>()> task(std::bind(&NodesParser::parse_thread, this));
    m_future = task.get_future();
    m_thread = std::thread(std::move(task));
}

void NodesParser::stop()
{
    LogF;
    m_stop_thread_flag = true;

    if (m_thread.joinable())
        m_thread.join();
}

std::shared_ptr<NodesManager> NodesParser::parse_thread()
{
    LogF;
    try
    {
        g::pOptsManager->pin_advanced_preload();

        auto nodes = o_json::parse(m_json_data);
        m_parsed_items_total = nodes.size();

        auto nodes_manager = std::make_shared<NodesManager>();
        prepare_parsers_map();

        std::unordered_map<std::string, std::list<hid>> models_map;

        const auto items = nodes.items();
        for (auto& [node_class, node_info] : items)
        {
            m_parsed_item++;
            parse_node(*nodes_manager.get(), node_class, node_info, models_map);

            if (m_stop_thread_flag)
            {
                Log("Parsing thread aborted");
                return {};
            }
        }

        g::pModelsManager->set_nodes_map(std::move(models_map));

        Log("Parsed: " << m_parsed_item << " item. Adding internal nodes");
        sort_nodes(*nodes_manager.get());
        nodes_manager->init_labels();
        Log("Nodes loading and parsing done");

        g::pOptsManager->pin_advanced_release();
        return nodes_manager;
    }
    catch (std::exception& e)
    {
        LogExcep(ExcepCategory::ComfyUI, "Parse error: " << e.what());
        g::pOptsManager->pin_advanced_release();
        m_parse_failed = true;
    }
    return {};
}

void NodesParser::parse_node(NodesManager& manager, const std::string& node_class, o_json& json_node, std::unordered_map<std::string, std::list<hid>>& models_map)
{
    try
    {
        Log("Node " << node_class << " parse: begin");
        auto node = std::make_unique<data::Node>(node_class);
        parse_names(*node.get(), node_class, json_node);        // name, display name, category, description

        //if (node_class == "KSampler")
        //    __debugbreak();

        parse_input(manager, *node.get(), json_node);        // input, input_order
        parse_output(manager, *node.get(), json_node);        // output, output_is_list, output_name, output_node, output_tooltips
        parse_others(*node.get(), json_node);                // python_module, experimental, deprecated, api_node

        node->finalize(models_map);
        put_in_tree(manager, node.get());

        auto ptr = node.get();
        if (!manager.m_nodes.try_emplace(node->get_id(), std::move(node)).second)
        {
            LogInvalidJson("Duplicate of node " << ptr->name);
        }
        else
            Log("Node " << node_class << " parse: end");
    }
    catch (std::exception& e)
    {
        LogExcep(ExcepCategory::ComfyUI, "Parse node '" << node_class << "' error: " << e.what());
    }
}

void NodesParser::parse_names(data::Node& node, const std::string& node_class, o_json& json_node)
{
    // name, display name, category, description

    node.node_class = node_class;
    if (json_node.contains("name") && json_node["name"].is_string())
        node.name = json_node["name"];
    if (node.name.empty())
        node.name = node_class;

    if (json_node.contains("display_name") && json_node["display_name"].is_string())
        node.display_name = json_node["display_name"];

    if (json_node.contains("description") && json_node["description"].is_string())
        node.description = json_node["description"];

    if (json_node.contains("category") && json_node["category"].is_string())
    {
        node.category = json_node["category"];
        if (node.category.empty())
            node.category = "_no_category";
    }
}

static std::optional<primitive> get_combo_item(const o_json& j)
{
    if (j.is_string())
        return std::string(j);
    if (j.is_boolean())
        return bool(j);
    if (j.is_number_float())
        return double(j);
    if (j.is_number_integer())
        return int64_t(j);
    if (j.is_null())
        return std::monostate();

    return std::nullopt;
}

template <typename TYPE>
static TYPE json_to_number(o_json& j)
{
    if (j.is_number())
    {
        return j.get<TYPE>();
    }
    if (j.is_boolean())
    {
        return j.get<bool>() ? 1 : 0;
    }
    if (j.is_string())
    {
        if (std::is_same_v<TYPE, int64_t>)
            return cast::to<int64_t, TYPE>(strtoll(j.get_ref<const std::string&>().c_str(), nullptr, 10));
        if (std::is_same_v<TYPE, double>)
            return cast::to<double, TYPE>(strtod(j.get_ref<const std::string&>().c_str(), nullptr));
    }
    if (j.is_null())
    {
        return {};
    }

    LogInternalLogicS("json data unknown type " << j.type_name() << " (number expected)");
    return {};
}

static std::string json_to_string(o_json& j)
{
    if (j.is_string())
    {
        return std::move(j.get_ref<std::string&>());
    }
    if (j.is_boolean())
    {
        return (j.get<bool>() ? "true" : "false");
    }
    if (j.is_number_integer())
    {
        return std::to_string(j.get<int64_t>());
    }
    if (j.is_number_float())
    {
        return std::to_string(j.get<double>());
    }
    if (j.is_null())
    {
        return {};
    }

    LogInternalLogicS("json data unknown type " << j.type_name() << " (string expected)");
    return {};
}

static bool json_to_bool(o_json& j)
{
    if (j.is_boolean())
    {
        return j.get<bool>();
    }
    if (j.is_number())
    {
        return (j.get<int64_t>() != 0);
    }
    if (j.is_string())
    {
        const auto& str = j.get_ref<const std::string&>();
        return (str == "1" || str == "true" || str == "True" || str == "TRUE");
    }
    if (j.is_null())
    {
        return false;
    }

    LogInternalLogicS("json data unknown type " << j.type_name() << " (bool expected)");
    return false;
}

static primitive get_from_pin_type(const data::Node& jnode, data::Pin& pin, o_json& j, const char* attr)
{
    switch (pin.def_state.type)
    {
    case data::PinType::INT:
    case data::PinType::NUMBER:
        return json_to_number<int64_t>(j);
    case data::PinType::FLOAT:
        return json_to_number<double>(j);
    case data::PinType::BOOLEAN:
        return json_to_bool(j);
    case data::PinType::STRING:
    case data::PinType::COLOR:
        return json_to_string(j);
    case data::PinType::COMBO:
        {
            auto value = get_combo_item(j);
            if (value.has_value())
                return std::move(value.value());
        }
        break;
    }

    LogUnhandledJsonS("Wrong pin type. node=" << jnode.name << ", pin=" << pin.def_state.name << ", attr=" << attr << ", type=" << j.type_name());
    return {};
}

void NodesParser::prepare_parsers_map()
{
    const auto log_err_type = [this](const data::Node& node, data::Pin& pin, const o_json& j, const char* attr)
    {
        LogUnhandledJson("Unknown type node=" << node.name << ", pin=" << pin.def_state.name << ", attr=" << attr << ", type=" << j.type_name());
    };

    const auto log_err_value = [this](const data::Node& node, data::Pin& pin, const std::string& value, const char* attr)
    {
        LogUnhandledJson("Unknown type node=" << node.name << ", pin=" << pin.def_state.name << ", attr=" << attr << ", value=" << value);
    };

    // sorted a-z

    m_parsers["advanced"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.advanced = j;
        else log_err_type(node, pin, j, "advanced");
    };
    m_parsers["audio_upload"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.audio_upload = j;
        else log_err_type(node, pin, j, "audio_upload");
    };
    m_parsers["control_after_generate"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.cag_require = j;
        else log_err_type(node, pin, j, "control_after_generate");
    };
    m_parsers["default"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        pin.def = get_from_pin_type(node, pin, j, "default");
    };
    m_parsers["defaultInput"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.def_input = j;
        else log_err_type(node, pin, j, "defaultInput");
    };
    m_parsers["display"] =
        [&log_err_type, &log_err_value](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_string())
        {
            const std::string& display = j.get_ref<const std::string&>();
            if (display == "color")
            {
                pin.display_as_color = true;
            }
            else if (display == "slider")
            {
                pin.display_as_slider = true;
            }
            else if (display != "number")
            {
                log_err_value(node, pin, display, "display");
            }
        }
        else
            log_err_type(node, pin, j, "defaultInput");
    };
    m_parsers["dynamicPrompts"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.dynamic_prompts = j;
        else log_err_type(node, pin, j, "dynamicPrompts");
    };
    m_parsers["file_upload"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.file_upload = j;
        else log_err_type(node, pin, j, "file_upload");
    };
    m_parsers["forceInput"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.force_input = j;
        else log_err_type(node, pin, j, "forceInput");
    };
    m_parsers["image_folder"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_string()) pin.image_folder = j;
        else log_err_type(node, pin, j, "image_folder");
    };
    m_parsers["image_upload"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.image_upload = j;
        else log_err_type(node, pin, j, "image_upload");
    };
    m_parsers["label_off"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_string()) pin.label_off = j;
        else log_err_type(node, pin, j, "label_off");
    };
    m_parsers["label_on"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_string()) pin.label_on = j;
        else log_err_type(node, pin, j, "label_on");
    };
    m_parsers["lazy"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.lazy = j;
        else log_err_type(node, pin, j, "lazy");
    };
    m_parsers["min"] =
        [](const data::Node& node, data::Pin& pin, o_json& j)
    {
        pin.min = get_from_pin_type(node, pin, j, "min");
    };
    m_parsers["max"] =
        [](const data::Node& node, data::Pin& pin, o_json& j)
    {
        pin.max = get_from_pin_type(node, pin, j, "max");
    };
    m_parsers["multiline"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.multiline = j;
        else log_err_type(node, pin, j, "multiline");
    };
    m_parsers["multiselect"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.multiselect = j;
        else log_err_type(node, pin, j, "multiselect");
    };
    m_parsers["options"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (data::PinType::COMBO == pin.def_state.type && j.is_array())
        {
            for (const auto& val : j)
            {
                auto value = get_combo_item(val);
                if (value.has_value())
                    pin.list.push_back(std::move(value.value()));
                else
                    log_err_type(node, pin, val, "COMBO/options/item");
            }
        }
        else
            log_err_type(node, pin, j, "options");
    };
    m_parsers["placeholder"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_string()) pin.placeholder = j;
        else log_err_type(node, pin, j, "placeholder");
    };
    m_parsers["round"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_number_float())
        {
            pin.round = double(j);
        }
        else if (j.is_boolean())
        {
            pin.do_round = bool(j);
        }
        else if (!j.is_null())
        {
            log_err_type(node, pin, j, "round");
        }

    };
    m_parsers["step"] =
        [](const data::Node& node, data::Pin& pin, o_json& j)
    {
        pin.step = get_from_pin_type(node, pin, j, "step");
    };
    m_parsers["tooltip"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_string()) pin.tooltip = j;
        else log_err_type(node, pin, j, "tooltip");
    };
    m_parsers["video_upload"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, o_json& j)
    {
        if (j.is_boolean()) pin.video_upload = j;
        else log_err_type(node, pin, j, "video_upload");
    };
}

void NodesParser::parse_by_key(const data::Node& node, data::Pin& pin, o_json& j, const std::string& key)
{
    const auto it = m_parsers.find(key);
    if (it == m_parsers.end())
    {
        LogUnhandledJson("Unknown key node=" << node.name << ", pin=" << pin.def_state.name << ", key=" << key);
    }
    else
    {
        it->second(node, pin, j);
    }
}

void NodesParser::parse_input(NodesManager& manager, data::Node& node, o_json& jnode)
{
    // input, input_order

    const auto parse_input = [&node, &manager, this](data::Node::Input& input, o_json& jobjects, bool is_required, bool is_optional, bool is_hidden)
    {
        auto items = jobjects.items();
        for (const auto&[pin_name, pin_info] : items)
        {
            auto pin = std::make_unique<data::Pin>(&node, pin_name);
            input.pins.push_back(pin.get());

            pin->input = true;
            pin->required = is_required;
            pin->optional = is_optional;
            pin->hidden = is_hidden;
            pin->def_state.name = pin_name;

            //Log("Node: " << node.name << ", Pin: " << pin_name);

            if (pin_info.is_array())
            {
                auto& jarr = pin_info.get_ref<o_json::array_t&>();
                if (jarr.size() == 1 && jarr[0].is_string())
                {
                    pin->initialize(jarr[0]);
                    if (data::PinType::CUSTOM == pin->def_state.type)
                    {
                        LogUnhandledJson("/input/CUSTOM=" << pin->def_state.type_str << ". Node=" << node.name << ", pin=" << pin->def_state.name);
                    }
                }
                else if (jarr.size() == 2 && jarr[0].is_string() && jarr[1].is_object())
                {
                    pin->initialize(jarr[0]);
                    if (data::PinType::CUSTOM == pin->def_state.type)
                    {
                        LogUnhandledJson("/input/CUSTOM=" << pin->def_state.type_str << ". Node=" << node.name << ", pin=" << pin->def_state.name);
                    }

                    for (auto&[key, value] : jarr[1].items())
                    {
                        parse_by_key(node, *pin, value, key);
                    }
                }
                else if (jarr.size() == 2 && jarr[0].is_array() && jarr[1].is_object() ||
                    jarr.size() == 1 && jarr[0].is_array())
                {
                    pin->initialize("COMBO");
                    for (const auto& item : jarr[0])
                    {
                        auto value = get_combo_item(item);
                        if (value.has_value())
                            pin->list.push_back(std::move(value.value()));
                        else
                        {
                            LogUnhandledJson("/input/<>/COMBO/item. Unknown item type. Node=" << node.name << ", pin="
                                << pin->def_state.name << ", type=" << item.type_name());
                        }
                    }
                    if (jarr.size() == 2)
                    {
                        for (const auto&[key, value] : jarr[1].items())
                        {
                            parse_by_key(node, *pin, value, key);
                        }
                    }
                }
                else
                {
                    LogUnhandledJson("/input/<>/pin_info. Unknown arr. Node=" << node.name << ", pin="
                        << pin->def_state.name << ", arr_sz=" << jarr.size());
                }
            }
            else
            {
                LogUnhandledJson("/input/<>/pin_info. Not array. Node=" << node.name << ", pin="
                    << pin->def_state.name << ", type=" << pin_info.type_name());
            }

            pin->finalize();
            auto ptr = pin.get();
            if (!manager.m_pins.try_emplace(ptr->get_id(), std::move(pin)).second)
            {
                LogUnhandledJson("/input/. Pin name duplicate. Node=" << node.name << ", pin=" << ptr->def_state.name << ", type=" << pin_info.type_name());
            }
            else if (ptr->cag_require)
            {
                auto cag = std::make_unique<data::Pin>(&node, ptr->def_state.name + "_control");
                input.pins.push_back(cag.get());

                cag->input = true;
                cag->required = is_required;
                cag->optional = is_optional;
                cag->hidden = is_hidden;
                cag->cag_pin = true;
                cag->def_state.name = "control";
                cag->initialize("COMBO");
                cag->finalize();

                ptr = cag.get();
                manager.m_pins.emplace(ptr->get_id(), std::move(cag));
            }
        }
    };

    if (jnode.contains("input"))
    {
        auto& input = jnode["input"];

        if (input.contains("required") && input["required"].is_object())
        {
            parse_input(node.required, input["required"], true, false, false);
        }
        if (input.contains("optional") && input["optional"].is_object())
        {
            parse_input(node.optional, input["optional"], false, true, false);
        }
        //if (input.contains("hidden") && input["hidden"].is_object())
        //{
        //    parse_input(node.hidden, input["hidden"], false, false, true);
        //}
    }

    const auto parse_order = [this, &node](data::Node::Input& input, const o_json& jarray)
    {
        for (const auto& item : jarray)
        {
            if (item.is_string())
                input.order.push_back(item);
            else
            {
                LogUnhandledJson("/input_order. Not a string. Node=" << node.name << ", type=" << item.type_name());
            }
        }
    };

    const auto sort_by_order = [](data::Node::Input& input)
    {
        if (input.order.size() != input.pins.size())
            return;

        std::vector<data::Pin*> sorted;
        sorted.reserve(input.pins.size());

        for (const auto& name : input.order)
        {
            for (auto pin : input.pins)
            {
                if (pin->def_state.name == name)
                {
                    sorted.push_back(pin);
                    break;
                }
            }
        }

        input.pins = std::move(sorted);
        input.order.clear();
    };

    if (jnode.contains("input_order"))
    {
        auto& order = jnode["input_order"];
        if (order.contains("required") && order["required"].is_array())
        {
            parse_order(node.required, order["required"]);
            sort_by_order(node.required);
        }
        if (order.contains("optional") && order["optional"].is_array())
        {
            parse_order(node.optional, order["optional"]);
            sort_by_order(node.optional);
        }
        //if (order.contains("hidden") && order["hidden"].is_array())
        //{
        //    parse_order(node.hidden, order["hidden"]);
        //    sort_by_order(node.hidden);
        //}
    }

    node.has_input = !node.required.pins.empty() || !node.optional.pins.empty() /*|| !node.hidden.pins.empty()*/;
}

void NodesParser::parse_output(NodesManager& manager, data::Node& node, o_json& jnode)
{
    // output, output_is_list, output_name, output_node, output_tooltips

    if (jnode.contains("output"))
    {
        auto& output = jnode["output"];
        if (output.is_array())
        {
            int64_t index = 0;
            for (const auto& item : output)
            {
                auto pin = std::make_unique<data::Pin>(&node, index++);
                pin->output = true;

                if (item.is_string())
                {
                    pin->initialize(item);
                    if (data::PinType::CUSTOM == pin->def_state.type)
                    {
                        LogUnhandledJson("/input/CUSTOM=" << pin->def_state.type_str << ". Node=" << node.name << ", pin=" << pin->def_state.name);
                    }
                }
                else if (item.is_array())
                {
                    pin->initialize("COMBO");
                    for (const auto& out_item : item)
                    {
                        if (out_item.is_string())
                            pin->list.push_back(std::string(out_item));
                        else
                        {
                            LogUnhandledJson("/output. Combo item not a string. Node=" << node.name << ", type=" << out_item.type_name());
                        }
                    }
                }
                else
                {
                    LogUnhandledJson("/output. Not a string. Node=" << node.name << ", type=" << item.type_name());
                    continue;
                }

                auto ptr = pin.get();
                node.output.push_back(pin.get());
                if (!manager.m_pins.emplace(pin->get_id(), std::move(pin)).second)
                {
                    LogUnhandledJson("/output/. Pin name duplicate. Node=" << node.name << ", pin=" << ptr->def_state.name << ", idx=" << index);
                }
            }
        }
    }

    using add_new_callback = std::function<bool(const o_json& item, data::Pin* pin)>;
    using add_def_callback = std::function<void(data::Pin* pin)>;

    const auto parse_output_attr = [this, &jnode, &node](const char* attr_name, add_new_callback new_cb, add_def_callback def_cb)
    {
        if (jnode.contains(attr_name))
        {
            auto& attr = jnode[attr_name];
            if (attr.is_array())
            {
                auto out_pins = node.output.begin();
                for (const auto& item : attr)
                {
                    if (out_pins == node.output.end())
                    {
                        LogInvalidJson("number of " << attr_name << " is greater than pins count. Node=" << node.name);
                        break;
                    }

                    auto pin = *out_pins;
                    ++out_pins;
                    if (!new_cb(item, pin))
                    {
                        LogInvalidJson(attr_name << " unknown type. Node=" << node.name);
                    }
                }

                if (out_pins != node.output.end())
                {
                    LogInvalidJson("number of " << attr_name << " is less than pins count. Node=" << node.name);
                    for (; out_pins != node.output.end(); ++out_pins)
                    {
                        def_cb(*out_pins);
                    }
                }
            }
            else
            {
                LogUnhandledJson("/" << attr_name << ". Not array. Node=" << node.name << ", type=" << attr.type_name());
            }
        }
    };

    parse_output_attr("output_is_list",
        [](const o_json& jitem, data::Pin* pin)
        {
            if (jitem.is_boolean())
            {
                pin->output_is_list = jitem;
                return true;
            }
            return false;
        },
        [](data::Pin* pin)
        {
            pin->output_is_list = false;
        }
    );

    parse_output_attr("output_name",
        [](const o_json& jitem, data::Pin* pin)
        {
            if (jitem.is_string())
            {
                pin->def_state.name = jitem;
                return true;
            }
            return false;
        },
        [](data::Pin*) {}
    );

    parse_output_attr("output_tooltips",
        [](const o_json& jitem, data::Pin* pin)
        {
            if (jitem.is_string())
            {
                pin->tooltip = jitem;
                return true;
            }
            return jitem.is_null();
        },
        [](data::Pin*) {}
    );

    if (jnode.contains("output_node"))
    {
        const auto& output_node = jnode["output_node"];
        if (output_node.is_boolean())
            node.output_node = output_node;
        else
        {
            LogInvalidJson("output_node not a bool. Node=" << node.name);
        }
    }

    for (const auto pin : node.output)
    {
        pin->finalize();
    }

    node.has_output = !node.output.empty();
}

void NodesParser::parse_others(data::Node& node, o_json& jnode)
{
    // python_module, experimental, deprecated, api_node

    const auto get_bool = [this, &node, &jnode](const char* json_name, bool& value)
    {
        if (!jnode.contains(json_name))
            return;
        const auto& json_value = jnode[json_name];
        if (json_value.is_boolean())
            value = json_value;
        else
        {
            LogInvalidJson(json_name << " not a bool. Node=" << node.name);
        }
    };

    const auto get_string = [this, &node, &jnode](const char* json_name, std::string& value)
    {
        if (!jnode.contains(json_name))
            return;
        const auto& json_value = jnode[json_name];
        if (json_value.is_string())
            value = json_value;
        else
        {
            LogInvalidJson(json_name << " not a string. Node=" << node.name);
        }
    };

    get_bool("experimental", node.experimental);
    get_bool("deprecated", node.deprecated);
    get_bool("api_node", node.api_node);
    get_string("python_module", node.python_module);

    for (const auto&[name, value] : jnode.items())
    {
        if (name == "input" ||
            name == "input_order" ||
            name == "output" ||
            name == "output_is_list" ||
            name == "output_name" ||
            name == "name" ||
            name == "display_name" ||
            name == "description" ||
            name == "python_module" ||
            name == "category" ||
            name == "output_node" ||
            name == "output_tooltips" ||
            name == "experimental" ||
            name == "deprecated" ||
            name == "api_node"
            )
        {
            continue;
        }

        LogUnhandledJson("/unhandled node's attr. Node=" << node.name << ", attr=" << name);
    }
}
void NodesParser::put_in_tree(NodesManager& manager, data::Node* node)
{
    const auto search_branch = [](std::vector<data::NodeItem*>* items, const std::string& category)
    {
        for (auto it = items->begin(); it != items->end(); ++it)
        {
            if (!(*it)->is_node && category == (*it)->name_or_category)
                return it;
        }
        return items->end();
    };

    const auto cats = tools::str::split(node->category, '/');
    std::vector<data::NodeItem*>* root = &manager.m_nodes_tree;

    auto cat = cats.begin();
    for (; cat != cats.end(); ++cat)
    {
        auto it = search_branch(root, *cat);
        if (it == root->end())
            break;
        root = &(*it)->children;
    }

    for (; cat != cats.end(); ++cat)
    {
        auto branch_ptr = std::make_unique<data::NodeItem>();
        auto branch = branch_ptr.get();
        manager.m_node_items.push_back(std::move(branch_ptr));

        root->push_back(branch);
        branch->is_node = false;
        branch->name_or_category = *cat;
        branch->name_or_category_len = branch->name_or_category.size();
        branch->search = tools::str::to_lower_ascii(branch->name_or_category);
        root = &branch->children;
    }

    auto branch_ptr = std::make_unique<data::NodeItem>();
    auto branch = branch_ptr.get();
    manager.m_node_items.push_back(std::move(branch_ptr));

    root->push_back(branch);
    branch->is_node = true;
    branch->name_or_category = (node->display_name.empty() ? node->name : node->display_name);
    branch->name_or_category_len = branch->name_or_category.size();
    branch->search = tools::str::to_lower_ascii(node->name + " " + node->display_name);
    branch->node = node;

    manager.m_nodes_plain.push_back(branch);
}

void NodesParser::sort_nodes(NodesManager& manager)
{
    std::function<void(std::vector<data::NodeItem*>& nodes)> sort_tree;
    sort_tree = [&sort_tree](std::vector<data::NodeItem*>& nodes)
    {
        NodesManager::sort_nodes(nodes);

        for (auto node : nodes)
        {
            sort_tree(node->children);
        }
    };

    sort_tree(manager.m_nodes_tree);
    NodesManager::sort_nodes(manager.m_nodes_plain);
}

} // namespace mngr
