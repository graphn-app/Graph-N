#include "core/managers/nodes_parser.h"
#include "core/managers/models_manager.h"
#include "core/managers/settings_manager.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin.h"
#include "ui/graph-n_flow.h"
#include "ui/custom_nodes/custom_js_node.h"

namespace mngr
{

NodesParser::~NodesParser() noexcept = default;

void NodesParser::check_status(size_t& parsed_item, size_t& parse_total)
{
    if (!m_parse_finished.load(std::memory_order_relaxed) && m_future.wait_for(0ms) == std::future_status::ready)
    {
        if (m_thread.joinable())
            m_thread.join();

        if (!m_stop_thread_flag.stop_requested())    // ignore if parse thread was stopped by user
        {
            auto copy = m_future.get();
            if (m_parse_failed.load(std::memory_order_relaxed))
            {
                Log("Parsed: fail");
            }
            else [[likely]]
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
        m_parse_finished.store(true, std::memory_order_relaxed);
    }

    parsed_item = m_parsed_item.load(std::memory_order_relaxed);
    parse_total = m_parsed_items_total.load(std::memory_order_relaxed);
}

void NodesParser::parse(std::string&& data)
{
    Log("To parse: " << data.size() << " bytes");
    stop_and_cleanup();
    m_parse_finished.store(false, std::memory_order_relaxed);
    m_parse_failed.store(false, std::memory_order_relaxed);
    m_stop_thread_flag = std::stop_source();
    m_json_data = std::move(data);

    m_parsed_item.store(0, std::memory_order_relaxed);
    m_parsed_items_total.store(0, std::memory_order_relaxed);

    std::packaged_task<std::shared_ptr<NodesManager>()> task([this] { return parse_thread(); });
    m_future = task.get_future();
    m_thread = std::thread(std::move(task));
}

void NodesParser::stop_and_cleanup()
{
    m_stop_thread_flag.request_stop();
    if (m_thread.joinable())
        m_thread.join();
}

std::shared_ptr<NodesManager> NodesParser::parse_thread()
{
    try
    {
        const auto j = o_json::parse(m_json_data);
        m_parsed_items_total.store(j.size(), std::memory_order_relaxed);

        auto nodes_manager = std::make_shared<NodesManager>();
        prepare_parsers_map();

        std::unordered_map<std::string, std::list<hid>> models_map;

        for (auto& [jclass, jinfo] : j.items())
        {
            m_parsed_item.fetch_add(1, std::memory_order_relaxed);
            parse_node(*nodes_manager.get(), jclass, jinfo, models_map);

            if (m_stop_thread_flag.stop_requested()) [[unlikely]]
            {
                Log("Parsing thread aborted");
                return {};
            }
        }

        g::pModelsManager->set_nodes_map(std::move(models_map));

        Log("Parsed: " << m_parsed_item.load(std::memory_order_relaxed) << " item. Adding internal nodes");
        add_internal_nodes(*nodes_manager.get());
        sort_nodes(*nodes_manager.get());
        apply_options(*nodes_manager.get());

        Log("All nodes loaded and parsed");

        return nodes_manager;
    }
    catch (const std::exception& ex)
    {
        LogExcep("Parse error: " << ex.what());
        m_parse_failed.store(true, std::memory_order_relaxed);
    }
    return {};
}

void NodesParser::parse_node(NodesManager& manager, const std::string& node_class, const o_json& jnode, std::unordered_map<std::string, std::list<hid>>& models_map)
{
    try
    {
        Log("Node " << node_class << " parse: begin");
        auto node = std::make_unique<data::Node>(node_class);
        parse_names(*node.get(), node_class, jnode);        // name, display name, category, description, search_aliases

        //BREAK_IF(node_class == "ControlNetPreprocessorSelector");

        parse_input(manager, *node.get(), jnode);        // input, input_order
        parse_output(manager, *node.get(), jnode);        // output, output_is_list, output_name, output_node, output_tooltips
        parse_others(*node.get(), jnode);                // python_module, experimental, deprecated, api_node

        g::pFlow->custom_js_nodes().test(node.get());
        node->finalize(&models_map);
        put_in_tree(manager, node.get());

        if (!manager.m_nodes.try_emplace(node->get_hash(), std::move(node)).second)
            LogWarn("Node " << node_class << ": skipped as duplicate");
        //else
        //    Log("Node " << node_class << ": parsed");
    }
    catch (const std::exception& ex)
    {
        LogExcep("Parse node '" << node_class << "' error: " << ex.what());
    }
}

void NodesParser::parse_names(data::Node& node, const std::string& node_class, const o_json& jnode)
{
    // name, display name, category, description

    if (jnode.contains("name") && jnode["name"].is_string())
        node.name = jnode["name"];
    if (node.name.empty())
        node.name = node_class;

    if (jnode.contains("display_name") && jnode["display_name"].is_string())
        node.display_name = jnode["display_name"];

    if (jnode.contains("description") && jnode["description"].is_string())
        node.description = jnode["description"];

    if (jnode.contains("search_aliases") && jnode["search_aliases"].is_string())
        node.search_aliases = jnode["search_aliases"];

    if (jnode.contains("category") && jnode["category"].is_string())
    {
        node.category = jnode["category"];
        if (node.category.empty())
            node.category = "_no_category";
    }
}

template <typename TYPE>
TYPE json_to_number(const o_json& j)
{
    if (j.is_number())
    {
        static_assert(std::same_as<number_t, double>, "Update this function if number_t changes");

        if constexpr (std::is_same_v<TYPE, number>)
            return j.get<double>();
        else
            return j.get<TYPE>();
    }
    if (j.is_boolean())
    {
        return j.get<bool>() ? 1 : 0;
    }
    if (j.is_string())
    {
        if constexpr (std::is_same_v<TYPE, int64_t>)
            return cast::to_i64(strtoll(j.get_ref<const std::string&>().c_str(), nullptr, 10));
        else if constexpr (std::is_same_v<TYPE, double>)
            return cast::to_double(strtod(j.get_ref<const std::string&>().c_str(), nullptr));
    }
    if (j.is_null())
    {
        return {};
    }

    LogRErrorS("json data unknown type " << j.type_name() << " (number expected)", TYPE());
}

namespace
{
std::optional<Primitive> get_combo_item(const o_json& j)
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

std::string json_to_string(const o_json& j)
{
    if (j.is_string())
    {
        return j.get<std::string>();
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

    LogRErrorS("Json data unknown type " << j.type_name() << " (string expected)", std::string());
}

bool json_to_bool(const o_json& j)
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

    LogRErrorS("Json data unknown type " << j.type_name() << " (bool expected)", false);
}

Primitive get_from_pin_type(const data::Node& jnode, const data::Pin& pin, const o_json& j, const char* attr)
{
    switch (pin.def_state.type)
    {
    case PinType::INT:
        return json_to_number<int64_t>(j);
    case PinType::FLOAT:
        return json_to_number<double>(j);
    case PinType::NUMBER:
        return json_to_number<number>(j);
    case PinType::BOOLEAN:
        return json_to_bool(j);
    case PinType::STRING:
    case PinType::COLOR:
        return json_to_string(j);
    case PinType::COMBO:
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
} // namespace

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
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::Advanced, j.get<bool>());
        else log_err_type(node, pin, j, "advanced");
    };
    m_parsers["audio_upload"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::AudioUpload, j.get<bool>());
        else log_err_type(node, pin, j, "audio_upload");
    };
    m_parsers["control_after_generate"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::CagRequire, j.get<bool>());
        else log_err_type(node, pin, j, "control_after_generate");
    };
    m_parsers["default"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        pin.def = get_from_pin_type(node, pin, j, "default");
    };
    m_parsers["defaultInput"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::DefInput, j.get<bool>());
        else log_err_type(node, pin, j, "defaultInput");
    };
    m_parsers["display"] =
        [&log_err_type, &log_err_value](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_string())
        {
            const std::string& display = j.get_ref<const std::string&>();
            if (display == "color")
            {
                pin.flags.add(PinObjFlags::DisplAsColor);
            }
            else if (display == "slider")
            {
                pin.flags.add(PinObjFlags::DisplAsSlider);
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
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::DynamicPrompts, j.get<bool>());
        else log_err_type(node, pin, j, "dynamicPrompts");
    };
    m_parsers["file_upload"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::FileUpload, j.get<bool>());
        else log_err_type(node, pin, j, "file_upload");
    };
    m_parsers["forceInput"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::ForceInput, j.get<bool>());
        else log_err_type(node, pin, j, "forceInput");
    };
    m_parsers["image_folder"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_string()) pin.image_folder = j;
        else log_err_type(node, pin, j, "image_folder");
    };
    m_parsers["image_upload"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::ImageUpload, j.get<bool>());
        else log_err_type(node, pin, j, "image_upload");
    };
    m_parsers["label_off"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_string()) pin.label_off = j;
        else log_err_type(node, pin, j, "label_off");
    };
    m_parsers["label_on"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_string()) pin.label_on = j;
        else log_err_type(node, pin, j, "label_on");
    };
    m_parsers["lazy"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::Lazy, j.get<bool>());
        else log_err_type(node, pin, j, "lazy");
    };
    m_parsers["min"] =
        [](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        pin.def_state.min = get_from_pin_type(node, pin, j, "min");
    };
    m_parsers["max"] =
        [](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        pin.def_state.max = get_from_pin_type(node, pin, j, "max");
    };
    m_parsers["multiline"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::Multiline, j.get<bool>());
        else log_err_type(node, pin, j, "multiline");
    };
    m_parsers["multiselect"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::Multiselect, j.get<bool>());
        else log_err_type(node, pin, j, "multiselect");
    };
    m_parsers["options"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (PinType::COMBO == pin.def_state.type && j.is_array())
        {
            for (const auto& jval : j)
            {
                auto value = get_combo_item(jval);
                if (value.has_value())
                    pin.combo_items.push_back(std::move(value.value()));
                else
                    log_err_type(node, pin, jval, "COMBO/options/item");
            }
        }
        else
            log_err_type(node, pin, j, "options");
    };
    m_parsers["placeholder"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_string()) pin.placeholder = j;
        else log_err_type(node, pin, j, "placeholder");
    };
    m_parsers["round"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
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
        [](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        pin.def_state.step = get_from_pin_type(node, pin, j, "step");
    };
    m_parsers["tooltip"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_string()) pin.tooltip = j;
        else log_err_type(node, pin, j, "tooltip");
    };
    m_parsers["video_upload"] =
        [&log_err_type](const data::Node& node, data::Pin& pin, const o_json& j)
    {
        if (j.is_boolean()) pin.flags.add_remove_if(PinObjFlags::VideoUpload, j.get<bool>());
        else log_err_type(node, pin, j, "video_upload");
    };
}

void NodesParser::parse_by_key(const data::Node& node, data::Pin& pin, const o_json& j, const std::string& key)
{
    const auto it = m_parsers.find(key);
    if (it != m_parsers.end())
    {
        it->second(node, pin, j);
    }
    else
    {
        LogUnhandledJson("Unknown key node=" << node.name << ", pin=" << pin.def_state.name << ", key=" << key);
    }
}

void NodesParser::parse_input(NodesManager& manager, data::Node& node, const o_json& jnode)
{
    // input, input_order

    const auto parse_input = [&node, &manager, this](std::vector<data::Pin*>& input, const o_json& jobjects, bool is_required, bool is_optional, bool /*is_hidden*/)
    {
        auto jitems = jobjects.items();
        for (const auto& [jpin, jinfo] : jitems)
        {
            auto pin = std::make_unique<data::Pin>(&node, jpin, is_required, is_optional/*, is_hidden*/);
            pin->def_state.name = jpin;

            //BREAK_IF(pin->def_state.name.find("rope") != -1);

            //Log("Node: " << node.name << ", Pin: " << pin_name);

            if (jinfo.is_array())
            {
                if (jinfo.size() == 1 && jinfo[0].is_string())
                {
                    pin->initialize(jinfo[0]);
                    if (PinType::CUSTOM == pin->def_state.type) [[unlikely]]
                    {
                        LogUnhandledJson("/input/CUSTOM=" << pin->def_state.type_str << ". Node=" << node.name << ", pin=" << pin->def_state.name);
                    }
                }
                else if (jinfo.size() == 2 && jinfo[0].is_string() && jinfo[1].is_object())
                {
                    pin->initialize(jinfo[0]);
                    if (PinType::CUSTOM == pin->def_state.type) [[unlikely]]
                    {
                        LogUnhandledJson("/input/CUSTOM=" << pin->def_state.type_str << ". Node=" << node.name << ", pin=" << pin->def_state.name);
                    }

                    for (auto& [jkey, jvalue] : jinfo[1].items())
                    {
                        parse_by_key(node, *pin, jvalue, jkey);
                    }
                }
                else if ((jinfo.size() == 2 && jinfo[0].is_array() && jinfo[1].is_object()) ||
                    (jinfo.size() == 1 && jinfo[0].is_array()))
                {
                    pin->initialize("COMBO");
                    for (const auto& jitem : jinfo[0])
                    {
                        auto value = get_combo_item(jitem);
                        if (value.has_value())
                            pin->combo_items.push_back(std::move(value.value()));
                        else [[unlikely]]
                        {
                            LogUnhandledJson("/input/<>/COMBO/item. Unknown item type. Node=" << node.name << ", pin="
                                << pin->def_state.name << ", type=" << jitem.type_name());
                        }
                    }
                    if (jinfo.size() == 2)
                    {
                        for (const auto& [jkey, jvalue] : jinfo[1].items())
                        {
                            parse_by_key(node, *pin, jvalue, jkey);
                        }
                    }
                }
                else [[unlikely]]
                {
                    LogUnhandledJson("/input/<>/pin_info. Unknown arr. Node=" << node.name << ", pin="
                        << pin->def_state.name << ", arr_sz=" << jinfo.size());
                }
            }
            else [[unlikely]]
            {
                LogUnhandledJson("/input/<>/pin_info. Not array. Node=" << node.name << ", pin="
                    << pin->def_state.name << ", type=" << jinfo.type_name());
            }

            pin->finalize();
            auto ptr = pin.get();
            if (!manager.m_pins.try_emplace(ptr->get_hash(), std::move(pin)).second)
            {
                LogInvalidJson("/input/. Pin name duplicate. Node=" << node.name << ", pin=" << ptr->def_state.name << ", type=" << jinfo.type_name());
            }
            else [[likely]]
            {
                input.push_back(ptr);
            }
        }
    };

    if (jnode.contains("input"))
    {
        auto& input = jnode["input"];

        if (input.contains("required") && input["required"].is_object())
        {
            parse_input(node.input, input["required"], true, false, false);
        }
        if (input.contains("optional") && input["optional"].is_object())
        {
            parse_input(node.input, input["optional"], false, true, false);
        }
        //if (input.contains("hidden") && input["hidden"].is_object())
        //{
        //    parse_input(node.hidden, input["hidden"], false, false, true);
        //}
    }

    //const auto parse_order = [this, &node](data::Node::Input& input, const o_json& jarray)
    //{
    //    for (const auto& item : jarray)
    //    {
    //        if (item.is_string())
    //            input.order.push_back(item);
    //        else
    //        {
    //            LogUnhandledJson("/input_order. Not a string. Node=" << node.name << ", type=" << item.type_name());
    //        }
    //    }
    //};

    //const auto sort_by_order = [](data::Node::Input& input)
    //{
    //    if (input.order.size() != input.pins.size())
    //        return;

    //    std::vector<data::Pin*> sorted;
    //    sorted.reserve(input.pins.size());

    //    for (const auto& name : input.order)
    //    {
    //        for (auto pin : input.pins)
    //        {
    //            if (pin->def_state.name == name)
    //            {
    //                sorted.push_back(pin);
    //                break;
    //            }
    //        }
    //    }

    //    input.pins = std::move(sorted);
    //    input.order.clear();
    //};

    //if (jnode.contains("input_order"))
    //{
    //    auto& order = jnode["input_order"];
    //    if (order.contains("required") && order["required"].is_array())
    //    {
    //        parse_order(node.required, order["required"]);
    //        sort_by_order(node.required);
    //    }
    //    if (order.contains("optional") && order["optional"].is_array())
    //    {
    //        parse_order(node.optional, order["optional"]);
    //        sort_by_order(node.optional);
    //    }
    //    //if (order.contains("hidden") && order["hidden"].is_array())
    //    //{
    //    //    parse_order(node.hidden, order["hidden"]);
    //    //    sort_by_order(node.hidden);
    //    //}
    //}

    node.flags.add_remove_if(NodeObjFlags::HasInput, !node.input.empty());
}

void NodesParser::parse_output(NodesManager& manager, data::Node& node, const o_json& jnode)
{
    // output, output_is_list, output_name, output_node, output_tooltips

    if (jnode.contains("output"))
    {
        auto& joutput = jnode["output"];
        if (joutput.is_array())
        {
            int32_t index = 0;
            for (const auto& jitem : joutput)
            {
                auto pin = std::make_unique<data::Pin>(&node, index++);
                pin->flags.add(PinObjFlags::Output);

                if (jitem.is_string())
                {
                    pin->initialize(jitem);
                    if (PinType::CUSTOM == pin->def_state.type) [[unlikely]]
                    {
                        LogUnhandledJson("/input/CUSTOM=" << pin->def_state.type_str << ". Node=" << node.name << ", pin=" << pin->def_state.name);
                    }
                }
                else if (jitem.is_array())
                {
                    pin->initialize("COMBO");
                    for (const auto& out_item : jitem)
                    {
                        if (out_item.is_string())
                            pin->combo_items.push_back(std::string(out_item));
                        else [[unlikely]]
                        {
                            LogUnhandledJson("/output. Combo item not a string. Node=" << node.name << ", type=" << out_item.type_name());
                        }
                    }
                }
                else [[unlikely]]
                {
                    LogUnhandledJson("/output. Not a string. Node=" << node.name << ", type=" << jitem.type_name());
                    continue;
                }

                pin->finalize();
                auto ptr = pin.get();
                node.output.push_back(pin.get());
                if (!manager.m_pins.emplace(pin->get_hash(), std::move(pin)).second) [[unlikely]]
                {
                    LogUnhandledJson("/output/. Pin name duplicate. Node=" << node.name << ", pin=" << ptr->def_state.name << ", idx=" << index);
                }
            }
        }
    }

    using add_new_callback = std::function<bool(const o_json& jitem, data::Pin* pin)>;
    using add_def_callback = std::function<void(data::Pin* pin)>;

    const auto parse_output_attr = [this, &jnode, &node](const char* attr_name, add_new_callback new_cb, add_def_callback def_cb)
    {
        if (jnode.contains(attr_name))
        {
            auto& jattr = jnode[attr_name];
            if (jattr.is_array())
            {
                auto out_pins = node.output.begin();
                for (const auto& jitem : jattr)
                {
                    if (out_pins == node.output.end()) [[unlikely]]
                    {
                        LogInvalidJson("number of " << attr_name << " is greater than pins count. Node=" << node.name);
                        break;
                    }

                    auto pin = *out_pins;
                    ++out_pins;
                    if (!new_cb(jitem, pin)) [[unlikely]]
                    {
                        LogInvalidJson(attr_name << " unknown type. Node=" << node.name);
                    }
                }

                if (out_pins != node.output.end()) [[unlikely]]
                {
                    LogInvalidJson("number of " << attr_name << " is less than pins count. Node=" << node.name);
                    for (; out_pins != node.output.end(); ++out_pins)
                    {
                        def_cb(*out_pins);
                    }
                }
            }
            else [[unlikely]]
            {
                LogUnhandledJson("/" << attr_name << ". Not array. Node=" << node.name << ", type=" << jattr.type_name());
            }
        }
    };

    parse_output_attr("output_is_list",
        [](const o_json& jitem, data::Pin* pin)
        {
            if (jitem.is_boolean())
            {
                pin->flags.add_remove_if(PinObjFlags::OutputIsList, jitem.get<bool>());
                return true;
            }
            return false;
        },
        [](data::Pin* pin)
        {
            pin->flags.remove(PinObjFlags::OutputIsList);
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
            node.flags.add_remove_if(NodeObjFlags::OutputNode, output_node.get<bool>());
        else
        {
            LogInvalidJson("output_node not a bool. Node=" << node.name);
        }
    }

    for (const auto pin : node.output)
    {
        pin->finalize();
    }

    node.flags.add_remove_if(NodeObjFlags::HasOutput, !node.output.empty());
}

void NodesParser::parse_others(data::Node& node, const o_json& jnode)
{
    // python_module, experimental, deprecated, api_node

    const auto get_bool = [this, &node, &jnode](const char* json_name)
    {
        if (!jnode.contains(json_name)) [[likely]]
            return false;
        const auto& jvalue = jnode[json_name];
        if (jvalue.is_boolean()) [[likely]]
            return jvalue.get<bool>();
        LogInvalidJson(json_name << " not a bool. Node=" << node.name);
        return false;
    };

    const auto get_string = [this, &node, &jnode](const char* json_name, std::string& value)
    {
        if (!jnode.contains(json_name))
            return;
        const auto& jvalue = jnode[json_name];
        if (jvalue.is_string())
            value = jvalue;
        else [[unlikely]]
        {
            LogInvalidJson(json_name << " not a string. Node=" << node.name);
        }
    };

    node.flags.add_remove_if(NodeObjFlags::Experimental, get_bool("experimental"));
    node.flags.add_remove_if(NodeObjFlags::Deprecated, get_bool("deprecated"));
    node.flags.add_remove_if(NodeObjFlags::ApiNode, get_bool("api_node"));
    node.flags.add_remove_if(NodeObjFlags::DevOnly, get_bool("dev_only"));
    get_string("python_module", node.python_module);

    constexpr std::string_view custom_module = "custom_nodes.";
    constexpr std::string_view vanilla_module = "nodes";
    if (node.python_module == vanilla_module)
        node.python_module = VANILLA;
    else if (node.python_module.starts_with(custom_module))
    {
        node.python_module = node.python_module.substr(custom_module.size());
        node.python_module_custom_node = true;
    }
    node.python_module_hash = base::HashID::make_hash(node.python_module);

    if (m_known_attributes.empty())
    {
        m_known_attributes = {
            "input", "input_order", "output", "output_is_list", "output_name",
            "name", "display_name", "description", "python_module", "category",
            "output_node", "output_tooltips", "experimental", "deprecated", "api_node"
        };
    }

    for (const auto&[name, value] : jnode.items())
    {
        if (!m_known_attributes.contains(name)) [[unlikely]]
        {
            LogUnhandledJson("/unhandled node's attr. Node=" << node.name << ", attr=" << name);
        }
    }
}

void NodesParser::put_in_tree(NodesManager& manager, data::NodeItem* leaf_item, const std::string& path, std::vector<data::NodeItem*>& tree, const char* separator)
{
    const auto search_branch = [](std::vector<data::NodeItem*>* items, const std::string& name) noexcept
    {
        for (auto it = items->begin(); it != items->end(); ++it)
        {
            if (!(*it)->is_node && name == (*it)->name_or_category)
                return it;
        }
        return items->end();
    };

    const auto parts = (path.empty() ? std::vector<std::string>{ "__unknown" } : tools::str::split(path, separator, true));
    std::vector<data::NodeItem*>* root = &tree;

    // search existing branch
    auto part = parts.begin();
    for (; part != parts.end(); ++part)
    {
        auto it = search_branch(root, *part);
        if (it == root->end())
            break;
        root = &(*it)->children;
    }

    // create new branches, if needed
    for (; part != parts.end(); ++part)
    {
        auto branch_ptr = std::make_unique<data::NodeItem>();
        auto branch = branch_ptr.get();
        manager.m_node_items.push_back(std::move(branch_ptr));

        root->push_back(branch);
        branch->is_node = false;
        branch->name_or_category = *part;
        branch->name_or_category_len = branch->name_or_category.size();
        branch->search = tools::str::to_lower_ascii(branch->name_or_category);

        root = &branch->children;
    }

    // add node to the end
    root->push_back(leaf_item);
}

void NodesParser::put_in_tree(NodesManager& manager, data::Node* node)
{
    if (nullptr == node) [[unlikely]]
        return;

    // new leaf
    auto leaf_ptr = std::make_unique<data::NodeItem>();
    auto leaf = leaf_ptr.get();
    manager.m_node_items.push_back(std::move(leaf_ptr));

    leaf->is_node = true;
    leaf->name_or_category = (node->display_name.empty() ? node->name : node->display_name);
    leaf->name_or_category_len = leaf->name_or_category.size();
    leaf->search = tools::str::to_lower_ascii(node->name + " " + node->display_name + " " + node->search_aliases + " " + node->python_module);
    leaf->node = node;

    // plain list
    manager.m_nodes_plain.push_back(leaf);

    // add to trees
    put_in_tree(manager, leaf, node->category, manager.m_category_tree, "/");
    put_in_tree(manager, leaf, node->python_module, manager.m_module_tree, ".");
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

    sort_tree(manager.m_category_tree);
    sort_tree(manager.m_module_tree);
    NodesManager::sort_nodes(manager.m_nodes_plain);
}

data::Node* add_internal_node(hid node_hash, std::unordered_map<hid, std::unique_ptr<data::Node>>& all_nodes)
{
    data::Node* node = nullptr;
    if (all_nodes.find(node_hash) == all_nodes.end())
    {
        auto node_ptr = data::CNI_BaseObj::cni_make_base_object(node_hash);
        node = node_ptr.get();
        all_nodes.emplace(node_hash, std::move(node_ptr));
        LogS("Custom node added: " << node->node_class);
    }
    return node;
}

void NodesParser::add_internal_nodes(NodesManager& manager)
{
    put_in_tree(manager, add_internal_node(js::REROUTE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::PRIMITIVE_NODE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::NOTE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::MARKDOWN_NOTE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::FAST_BYPASSER_RGTHREE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::FAST_MUTER_RGTHREE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::FAST_GROUPS_MUTER_RGTHREE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::FAST_GROUPS_BYPASSER_RGTHREE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::FAST_ACTIONS_BUTTON_RGTHREE, manager.m_nodes));
    put_in_tree(manager, add_internal_node(js::BOOKMARK_RGTHREE, manager.m_nodes));
}

template <size_t N>
struct CompileTimeSet
{
    std::array<hid, N> data;
    constexpr bool contains(hid hash) const noexcept
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (data[i] == hash)
                return true;
        }
        return false;
    }
};

template <typename... Args>
CompileTimeSet(Args...) -> CompileTimeSet<sizeof...(Args)>;

void NodesParser::apply_options(NodesManager& manager)
{
    g::pOptsManager->items_pins_preload_all();
    g::pOptsManager->items_nodes_preload_all();

    for (auto& [hash, pin] : manager.m_pins)
    {
        const auto& opt = g::pOptsManager->items_pin_load(hash);
        pin->flags.add_raw(opt.flags.get_raw());
    }

    using namespace tools::str;

    constexpr CompileTimeSet restricted_nodes =
    {
        hash_stable("CheckpointLoaderSimple"),
        hash_stable("CheckpointLoader"),
        hash_stable("CheckpointLoaderSimpleShared //Inspire"),
        hash_stable("StableCascade_CheckpointLoader //Inspire"),
        hash_stable("unCLIPCheckpointLoader"),
        hash_stable("ImageOnlyCheckpointLoader"),
        hash_stable("LoraLoader"),
        hash_stable("LoraLoaderModelOnly"),
        hash_stable("LoraLoaderBypass"),
        hash_stable("LoraLoaderBypassModelOnly"),
        hash_stable("ACN_CtrLoRALoader"),
        hash_stable("LoraLoaderBlockWeight //Inspire"),
        hash_stable("VAELoader"),
        hash_stable("LTXVAudioVAELoader"),
        hash_stable("WanVideoVAELoader"),
        hash_stable("WanVideoTinyVAELoader"),
        hash_stable("OviMMAudioVAELoader"),
        hash_stable("ControlNetLoader"),
        hash_stable("ControlNetLoaderAdvanced"),
        hash_stable("DiffControlNetLoader"),
        hash_stable("DiffControlNetLoaderAdvanced"),
        hash_stable("ACN_ControlNetLoaderAdvanced"),
        hash_stable("ACN_DiffControlNetLoaderAdvanced"),
        hash_stable("WanVideoUni3C_ControlnetLoader"),
        hash_stable("WanVideoControlnetLoader"),
        hash_stable("UNETLoader"),
        hash_stable("CLIPLoader"),
        hash_stable("DualCLIPLoader"),
        hash_stable("TripleCLIPLoader"),
        hash_stable("QuadrupleCLIPLoader"),
        hash_stable("CLIPVisionLoader"),
        hash_stable("UpscaleModelLoader"),
        hash_stable("LatentUpscaleModelLoader"),
        hash_stable("StyleModelLoader"),
        hash_stable("GLIGENLoader"),
        hash_stable("HypernetworkLoader"),
        hash_stable("PhotomakerLoader"),
        hash_stable("SAMLoader"),
        hash_stable("SAMDetectorSegmented"),
    };

    constexpr CompileTimeSet display_output_images =
    {
        hash_stable("PreviewImage"),
        hash_stable("SaveImage"),
        hash_stable("SaveImageWebsocket"),
        hash_stable("ImageSave"),
        hash_stable("ImageSaveExtended"),
        hash_stable("Image Save"),
    };
    constexpr CompileTimeSet display_output_videos =
    {
        hash_stable("SaveVideo"),
        hash_stable("VHS_VideoCombine"),
        hash_stable("SaveAnimatedWEBP"),
        hash_stable("SaveAnimatedPNG"),
    };
    constexpr CompileTimeSet display_output_audios =
    {
        hash_stable("SaveAudio"),
        hash_stable("SaveAudioMP3"),
        // hash_stable("SaveAudioOpus"),        // TODO
        hash_stable("PreviewAudio"),
    };
    constexpr CompileTimeSet display_output_texts =
    {
        hash_stable("PreviewAny"),
        hash_stable("DisplayAny"),
        hash_stable("ShowText|pysssss"),
        hash_stable("ShowText"),
        hash_stable("CR Text"),
        hash_stable("Preview Text"),
    };

    for (auto& [hash, node] : manager.m_nodes)
    {
        auto& opt = g::pOptsManager->items_node_load(hash);
        node->flags.add_raw(opt.flags.get_raw());

        if (node->flags.has_any(NodeObjFlags::CustomMediaFromSource, NodeObjFlags::CustomMediaUploadButton) &&
            node->flags.has_none(NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaImage))
        {
            node->flags.remove(NodeObjFlags::CustomMediaFromSource, NodeObjFlags::CustomMediaUploadButton);
        }

        if (restricted_nodes.contains(hash) || node->flags.has(NodeObjFlags::CustomMediaFromOutput))
        {
            node->flags.add(NodeObjFlags::RestrictCfg);
            node->flags.remove(NodeObjFlags::CustomMediaFromSource, NodeObjFlags::CustomMediaUploadButton);
        }

        if (display_output_images.contains(hash))
        {
            node->flags.remove(NodeObjFlags::CustomMediaFromSource, NodeObjFlags::CustomMediaUploadButton, NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaText);
            node->flags.add(NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaFromOutput, NodeObjFlags::RestrictCfg);
        }
        else if (display_output_videos.contains(hash))
        {
            node->flags.remove(NodeObjFlags::CustomMediaFromSource, NodeObjFlags::CustomMediaUploadButton, NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaText);
            node->flags.add(NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaFromOutput, NodeObjFlags::RestrictCfg);
        }
        else if (display_output_audios.contains(hash))
        {
            node->flags.remove(NodeObjFlags::CustomMediaFromSource, NodeObjFlags::CustomMediaUploadButton, NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaText);
            node->flags.add(NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaFromOutput, NodeObjFlags::RestrictCfg);
        }
        else if (display_output_texts.contains(hash))
        {
            node->flags.remove(NodeObjFlags::CustomMediaFromSource, NodeObjFlags::CustomMediaUploadButton, NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaText);
            node->flags.add(NodeObjFlags::CustomMediaText, NodeObjFlags::CustomMediaFromOutput, NodeObjFlags::RestrictCfg);
        }
        else if (!opt.display_source.empty() && node->flags.has_not(NodeObjFlags::RestrictCfg))
        {
            for (auto pin : node->input)
            {
                if (pin->def_state.name == opt.display_source)
                {
                    node->custom_widget_source = pin;
                    break;
                }
            }

            // source pin name changed?
            if (nullptr == node->custom_widget_source && node->flags.has_any(NodeObjFlags::CustomMediaFromSource, NodeObjFlags::CustomMediaUploadButton))
            {
                for (auto pin : node->input)
                {
                    if (PinType::COMBO == pin->def_state.type)
                    {
                        if (node->custom_widget_source != nullptr)
                        {
                            node->custom_widget_source = nullptr;
                            break;
                        }
                        node->custom_widget_source = pin;
                    }
                }
            }
        }

        if (node->flags.has_any_by_mask_obj(g_node_obj_all_custom_widget_mask))
            node->flags.add(NodeObjFlags::CustomMediaWidget);
    }

    g::pOptsManager->items_pins_release_all();
    g::pOptsManager->items_nodes_release_all();
}

} // namespace mngr
