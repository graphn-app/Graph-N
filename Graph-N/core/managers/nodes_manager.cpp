
#include "core/managers/nodes_manager.h"
#include "core/data/node.h"
#include "core/data/implemented_nodes/note.h"
#include "core/data/implemented_nodes/markdown_note.h"

namespace mngr
{

//////////////////////////////////////////////////////////////////
//////////    MANAGER          ///////////////////////////////////
//////////////////////////////////////////////////////////////////

void NodesManager::free()
{
	m_nodes_by_id.clear();
	m_nodes_by_name.clear();
	m_nodes_sorted.clear();
	m_pins.clear();
	m_nodes_tree.clear();
}

void NodesManager::init_from_copy(NodesManager& copy)
{
	free();
	m_nodes_by_id = std::move(copy.m_nodes_by_id);
	m_nodes_by_name = std::move(copy.m_nodes_by_name);
	m_nodes_sorted = std::move(copy.m_nodes_sorted);
	m_pins = std::move(copy.m_pins);
	m_nodes_tree = std::move(copy.m_nodes_tree);
}

std::shared_ptr<data::Node> NodesManager::find_node(lid id) const
{
	const auto it = m_nodes_by_id.find(id);
	if (it == m_nodes_by_id.end())
		return nullptr;
	return it->second;
}

std::shared_ptr<data::Pin> NodesManager::find_pin(lid id) const
{
	const auto it = m_pins.find(id);
	if (it == m_pins.end())
		return nullptr;
	return it->second;
}

//////////////////////////////////////////////////////////////////
//////////    PARSER           ///////////////////////////////////
//////////////////////////////////////////////////////////////////

void NodesParser::check_status(size_t& parsed_item, size_t& parse_total)
{
	using namespace std::chrono_literals;
	if (!m_parse_finished && m_future.wait_for(1ms) == std::future_status::ready)
	{
		if (m_thread.joinable())
			m_thread.join();

		if (!m_stop_thread_flag)	// ignore if parse thread was stopped by user
		{
			auto copy = m_future.get();
			if (m_parse_failed)
			{
				Log("Parsed: fail");
			}
			else
			{
				// this is UI thread, so it is safe to recreate on the fly
				g::pNodesManager->init_from_copy(copy);
				Log("Parsed: ok");
			}
		}
		m_stop_thread_flag = false;
		m_parse_finished = true;
	}

	parsed_item = m_parsed_item;
	parse_total = m_parsed_items_total;
}

void NodesParser::parse(std::string data)
{
	stop();
	m_parse_finished = false;
	m_parse_failed = false;
	m_stop_thread_flag = false;
	m_json_data = std::move(data);

	m_parsed_item = 0;
	m_parsed_items_total = 0;

	std::packaged_task<NodesManager()> task(std::bind(&NodesParser::parse_thread, this));
	m_future = task.get_future();
	m_thread = std::thread(std::move(task));
}

void NodesParser::stop()
{
	m_stop_thread_flag = true;

	if (m_thread.joinable())
		m_thread.join();
}

NodesManager NodesParser::parse_thread()
{
	try
	{
		auto nodes = nlohmann::json::parse(m_json_data);
		m_parsed_items_total = nodes.size();

		NodesManager copy;
		prepare_parsers_map();

		const auto items = nodes.items();
		for (auto& [node_class, node_info] : items)
		{
			m_parsed_item++;
			parse_node(copy, node_class, node_info);

			if (m_stop_thread_flag)
			{
				Log("Parsing thread aborted");
				return {};
			}
		}

		add_internal_nodes(copy);
		sort_nodes_tree(copy);
		return copy;
	}
	catch (std::exception& e)
	{
		LogExcep("Parse error: " << e.what());
		m_parse_failed = true;
	}
	return {};
}

void NodesParser::parse_node(NodesManager& manager, const std::string& node_class, nlohmann::json& json_node)
{
	auto node = std::make_shared<data::Node>();

	try
	{
		parse_names(*node.get(), node_class, json_node);		// name, display name, category, description

#ifdef _DEBUG
		if (node->name == "")
		{
			__debugbreak();
		}
#endif

		parse_input(manager, *node.get(), json_node);		// input, input_order
		parse_output(manager, *node.get(), json_node);		// output, output_is_list, output_name, output_node, output_tooltips
		parse_others(*node.get(), json_node);				// python_module, experimental, deprecated, api_node

		put_in_tree(manager, node);
		manager.m_nodes_by_id.emplace(node->get_id(), node);
	}
	catch (std::exception& e)
	{
		LogExcep("Parse node '" << node->name <<  "' error: " << e.what());
	}
}

void NodesParser::parse_names(data::Node& node, const std::string& node_class, nlohmann::json& json_node)
{
	// name, display name, category, description

	if (json_node.contains("name") && json_node["name"].is_string())
		node.name = json_node["name"];
	if (node.name != node_class)
		node.node_class = node_class;

	if (json_node.contains("display_name") && json_node["display_name"].is_string())
		node.display_name = json_node["display_name"];

	if (json_node.contains("description") && json_node["description"].is_string())
		node.description = json_node["description"];

	if (json_node.contains("category") && json_node["category"].is_string())
		node.category = json_node["category"];
}

static std::optional<primitive> get_combo_item(const nlohmann::json& json)
{
	if (json.is_string())
		return std::string(json);
	if (json.is_boolean())
		return bool(json);
	if (json.is_number_float())
		return double(json);
	if (json.is_number_integer())
		return int64_t(json);
	if (json.is_null())
		return std::monostate();
	
	return std::nullopt;
}

template <typename TYPE>
static TYPE json_to_number(nlohmann::json& data)
{
	if (data.is_number())
	{
		return data.get<TYPE>();
	}
	if (data.is_boolean())
	{
		return cast::to<int64_t, TYPE>(cast::to_i64(data.get<bool>() != 0));
	}
	if (data.is_string())
	{
		if (std::is_same_v<TYPE, int64_t>)
			return cast::to<int64_t, TYPE>(strtoll(data.get_ref<const std::string&>().c_str(), nullptr, 10));
		if (std::is_same_v<TYPE, double>)
			return cast::to<double, TYPE>(strtod(data.get_ref<const std::string&>().c_str(), nullptr));
	}
	if (data.is_null())
	{
		return {};
	}

	LogInternalWarn("json data unknown type " << data.type_name() << " (number expected)");
	return {};
}

static std::string json_to_string(nlohmann::json& data)
{
	if (data.is_string())
	{
		return std::move(data.get_ref<std::string&>());
	}
	if (data.is_boolean())
	{
		return (data.get<bool>() ? "true" : "false");
	}
	if (data.is_number_integer())
	{
		return std::to_string(data.get<int64_t>());
	}
	if (data.is_number_float())
	{
		return std::to_string(data.get<double>());
	}
	if (data.is_null())
	{
		return {};
	}

	LogInternalWarn("json data unknown type " << data.type_name() << " (string expected)");
	return {};
}

static bool json_to_bool(nlohmann::json& data)
{
	if (data.is_boolean())
	{
		return data.get<bool>();
	}
	if (data.is_number())
	{
		return (data.get<int64_t>() != 0);
	}
	if (data.is_string())
	{
		const auto& str = data.get_ref<const std::string&>();
		return (str == "1" || str == "true" || str == "True" || str == "TRUE");
	}
	if (data.is_null())
	{
		return false;
	}

	LogInternalWarn("json data unknown type " << data.type_name() << " (bool expected)");
	return false;
}

static primitive get_from_pin_type(const data::Node& node, data::Pin& pin, nlohmann::json& json, const char* attr)
{
	switch (pin.type)
	{
	case data::PinType::INT:
		return json_to_number<int64_t>(json);
	case data::PinType::FLOAT:
		return json_to_number<double>(json);
	case data::PinType::BOOLEAN:
		return json_to_bool(json);
	case data::PinType::STRING:
	case data::PinType::COLOR:
		return json_to_string(json);
	case data::PinType::COMBO:
		{
			auto value = get_combo_item(json);
			if (value.has_value())
				return std::move(value.value());
		}
		break;
	}

	LogUnhandledJson("Wrong pin type. node=" << node.name << ", pin=" << pin.name << ", attr=" << attr << ", type=" << json.type_name());
	return {};
}

void NodesParser::prepare_parsers_map()
{
	const auto log_err_type = [](const data::Node& node, data::Pin& pin, const nlohmann::json& json, const char* attr)
	{
		LogUnhandledJson("Unknown type node=" << node.name << ", pin=" << pin.name << ", attr=" << attr << ", type=" << json.type_name());
	};

	const auto log_err_value = [](const data::Node& node, data::Pin& pin, const std::string& value, const char* attr)
	{
		LogUnhandledJson("Unknown type node=" << node.name << ", pin=" << pin.name << ", attr=" << attr << ", value=" << value);
	};

	// sorted a-z

	m_parsers["advanced"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.advanced = json;
		else log_err_type(node, pin, json, "advanced");
	};
	m_parsers["audio_upload"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.audio_upload = json;
		else log_err_type(node, pin, json, "audio_upload");
	};
	m_parsers["control_after_generate"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.control_after_generate = json;
		else log_err_type(node, pin, json, "control_after_generate");
	};
	m_parsers["default"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		pin.def = get_from_pin_type(node, pin, json, "default");
	};
	m_parsers["defaultInput"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.def_input = json;
		else log_err_type(node, pin, json, "defaultInput");
	};
	m_parsers["display"] =
		[&log_err_type, &log_err_value](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_string())
		{
			const std::string& display = json.get_ref<const std::string&>();
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
			log_err_type(node, pin, json, "defaultInput");
	};
	m_parsers["dynamicPrompts"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.dynamic_prompts = json;
		else log_err_type(node, pin, json, "dynamicPrompts");
	};
	m_parsers["file_upload"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.file_upload = json;
		else log_err_type(node, pin, json, "file_upload");
	};
	m_parsers["forceInput"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.force_input = json;
		else log_err_type(node, pin, json, "forceInput");
	};
	m_parsers["image_folder"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_string()) pin.image_folder = json;
		else log_err_type(node, pin, json, "image_folder");
	};
	m_parsers["image_upload"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.image_upload = json;
		else log_err_type(node, pin, json, "image_upload");
	};
	m_parsers["label_off"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_string()) pin.label_off = json;
		else log_err_type(node, pin, json, "label_off");
	};
	m_parsers["label_on"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_string()) pin.label_on = json;
		else log_err_type(node, pin, json, "label_on");
	};
	m_parsers["lazy"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.lazy = json;
		else log_err_type(node, pin, json, "lazy");	
	};
	m_parsers["min"] =
		[](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		pin.min = get_from_pin_type(node, pin, json, "min");
	};
	m_parsers["max"] =
		[](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		pin.max = get_from_pin_type(node, pin, json, "max");
	};
	m_parsers["multiline"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.multiline = json;
		else log_err_type(node, pin, json, "multiline");
	};
	m_parsers["multiselect"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.multiselect = json;
		else log_err_type(node, pin, json, "multiselect");
	};
	m_parsers["options"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (data::PinType::COMBO == pin.type && json.is_array())
		{
			for (const auto& val : json)
			{
				auto value = get_combo_item(val);
				if (value.has_value())
					pin.list.emplace_back(std::move(value.value()));
				else
					log_err_type(node, pin, val, "COMBO/options/item");
			}
		}
		else
			log_err_type(node, pin, json, "options");
	};
	m_parsers["placeholder"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_string()) pin.placeholder = json;
		else log_err_type(node, pin, json, "placeholder");
	};
	m_parsers["round"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_number_float())
		{
			pin.round = double(json);
		}
		else if (json.is_boolean())
		{
			pin.do_round = bool(json);
		}
		else if (!json.is_null())
		{
			log_err_type(node, pin, json, "round");
		}

	};
	m_parsers["step"] =
		[](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		pin.step = get_from_pin_type(node, pin, json, "step");
	};
	m_parsers["tooltip"] = 
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json) 
	{
		if (json.is_string()) pin.tooltip = json;
		else log_err_type(node, pin, json, "tooltip");
	};
	m_parsers["video_upload"] =
		[&log_err_type](const data::Node& node, data::Pin& pin, nlohmann::json& json)
	{
		if (json.is_boolean()) pin.video_upload = json;
		else log_err_type(node, pin, json, "video_upload");
	};
}

void NodesParser::parse_by_key(const data::Node& node, data::Pin& pin, nlohmann::json& json, const std::string& key)
{
	const auto it = m_parsers.find(key);
	if (it == m_parsers.end())
	{
		LogUnhandledJson("Unknown key node=" << node.name << ", pin=" << pin.name << ", key=" << key);
	}
	else
	{
		it->second(node, pin, json);
	}
}

void NodesParser::parse_input(NodesManager& manager, data::Node& node, nlohmann::json& json_node)
{
	// input, input_order

	using json = nlohmann::json;

	const auto parse_input = [&node, &manager, this](data::Node::Input& input, json& objects)
	{
		auto items = objects.items();
		for (const auto&[pin_name, pin_info] : items)
		{
			auto pin = std::make_shared<data::Pin>();
			input.pins.push_back(pin);
			manager.m_pins[pin->get_id()] = pin;

			pin->input = true;
			pin->name = pin_name;

			//Log("Node: " << node.name << ", Pin: " << pin_name);

			if (pin_info.is_array())
			{
				auto& arr = pin_info.get_ref<json::array_t&>();
				if (arr.size() == 1 && arr[0].is_string())
				{
					pin->initialize(arr[0]);
					if (data::PinType::CUSTOM == pin->type)
					{
						LogUnhandledJson("/input/CUSTOM=" << pin->type_str << ". Node=" << node.name << ", pin=" << pin->name);
					}
				}
				else if (arr.size() == 2 && arr[0].is_string() && arr[1].is_object())
				{
					pin->initialize(arr[0]);
					if (data::PinType::CUSTOM == pin->type)
					{
						LogUnhandledJson("/input/CUSTOM=" << pin->type_str << ". Node=" << node.name << ", pin=" << pin->name);
					}

					for (auto&[key, value] : arr[1].items())
					{
						parse_by_key(node, *pin, value, key);
					}
				}
				else if (arr.size() == 2 && arr[0].is_array() && arr[1].is_object() ||
						 arr.size() == 1 && arr[0].is_array())
				{
					pin->type = data::PinType::COMBO;
					for (const auto& item : arr[0])
					{
						auto value = get_combo_item(item);
						if (value.has_value())
							pin->list.emplace_back(std::move(value.value()));
						else
						{
							LogUnhandledJson("/input/<>/COMBO/item. Unknown item type. Node=" << node.name << ", pin="
								<< pin->name << ", type=" << item.type_name());
						}
					}
					if (arr.size() == 2)
					{
						for (const auto&[key, value] : arr[1].items())
						{
							parse_by_key(node, *pin, value, key);
						}
					}
				}
				else
				{
					LogUnhandledJson("/input/<>/pin_info_arr. Unknown arr. Node=" << node.name << ", pin=" 
						<< pin->name << ", arr_sz=" << arr.size());
				}
			}
			else
			{
				LogUnhandledJson("/input/<>/pin_info. Not array. Node=" << node.name << ", pin="
					<< pin->name << ", type=" << pin_info.type_name());
			}

			pin->finalize();
		}
	};

	if (json_node.contains("input"))
	{
		auto& input = json_node["input"];

		if (input.contains("required") && input["required"].is_object())
		{
			parse_input(node.required, input["required"]);
		}
		if (input.contains("optional") && input["optional"].is_object())
		{
			parse_input(node.optional, input["optional"]);
		}
		//if (input.contains("hidden") && input["hidden"].is_object())
		//{
		//	parse_input(node.hidden, input["hidden"]);
		//}
	}

	const auto parse_order = [&node](data::Node::Input& input, const json& array)
	{
		for (const auto& item : array)
		{
			if (item.is_string())
				input.order.emplace_back(item);
			else
			{
				LogUnhandledJson("/input_order. Not a string. Node=" << node.name << ", type=" << item.type_name());
			}
		}
	};

	if (json_node.contains("input_order"))
	{
		auto& order = json_node["input_order"];
		if (order.contains("required") && order["required"].is_array())
			parse_order(node.required, order["required"]);
		if (order.contains("optional") && order["optional"].is_array())
			parse_order(node.optional, order["optional"]);
		//if (order.contains("hidden") && order["hidden"].is_array())
		//	parse_order(node.hidden, order["hidden"]);
	}

	node.has_input = !node.required.pins.empty() || !node.optional.pins.empty() /*|| !node.hidden.pins.empty()*/;
}

void NodesParser::parse_output(NodesManager& manager, data::Node& node, nlohmann::json& json_node)
{
	// output, output_is_list, output_name, output_node, output_tooltips

	if (json_node.contains("output"))
	{
		auto& output = json_node["output"];
		if (output.is_array())
		{
			for (const auto& item : output)
			{
				auto pin = std::make_shared<data::Pin>();
				pin->output = true;

				if (item.is_string())
				{
					pin->initialize(item);
					if (data::PinType::CUSTOM == pin->type)
					{
						LogUnhandledJson("/input/CUSTOM=" << pin->type_str << ". Node=" << node.name << ", pin=" << pin->name);
					}
				}
				else if (item.is_array())
				{
					pin->type = data::PinType::COMBO;
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

				node.output.push_back(pin);
				manager.m_pins[pin->get_id()] = pin;
			}
		}
	}

	using add_new_callback = std::function<bool(const nlohmann::json& item, data::Pin* pin)>;
	using add_def_callback = std::function<void(data::Pin* pin)>;

	const auto parse_output_attr = [&json_node, &node](const char* attr_name, add_new_callback new_cb, add_def_callback def_cb)
	{
		if (json_node.contains(attr_name))
		{
			auto& attr = json_node[attr_name];
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
					if (!new_cb(item, pin.get()))
					{
						LogInvalidJson(attr_name << " unknown type. Node=" << node.name);
					}
				}

				if (out_pins != node.output.end())
				{
					LogInvalidJson("number of " << attr_name << " is less than pins count. Node=" << node.name);
					for ( ; out_pins != node.output.end(); ++out_pins)
					{
						def_cb(out_pins->get());
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
		[](const nlohmann::json& item, data::Pin* pin)
			{
				if (item.is_boolean())
				{
					pin->output_is_list = item;
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
		[](const nlohmann::json& item, data::Pin* pin)
		{
			if (item.is_string())
			{
				pin->name = item;
				return true;
			}
			return false;
		},
		[](data::Pin*) {}
	);

	parse_output_attr("output_tooltips",
		[](const nlohmann::json& item, data::Pin* pin)
		{
			if (item.is_string())
			{
				pin->tooltip = item;
				return true;
			}
			return item.is_null();
		},
		[](data::Pin*) {}
	);

	if (json_node.contains("output_node"))
	{
		const auto& output_node = json_node["output_node"];
		if (output_node.is_boolean())
			node.output_node = output_node;
		else
		{
			LogInvalidJson("output_node not a bool. Node=" << node.name);
		}
	}

	node.has_output = !node.output.empty();
}

void NodesParser::parse_others(data::Node& node, nlohmann::json& json_node)
{
	// python_module, experimental, deprecated, api_node

	const auto get_bool = [&node, &json_node](const char* json_name, bool& value)
	{
		if (!json_node.contains(json_name))
			return;
		const auto& json_value = json_node[json_name];
		if (json_value.is_boolean())
			value = json_value;
		else
		{
			LogInvalidJson(json_name << " not a bool. Node=" << node.name);
		}
	};

	const auto get_string = [&node, &json_node](const char* json_name, std::string& value)
	{
		if (!json_node.contains(json_name))
			return;
		const auto& json_value = json_node[json_name];
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

	for (const auto& [name, value] : json_node.items())
	{
		if (name == "input"				||
			name == "input_order"		||
			name == "output"			||
			name == "output_is_list"	||
			name == "output_name"		||
			name == "name"				||
			name == "display_name"		||
			name == "description"		||
			name == "python_module"		||
			name == "category"			||
			name == "output_node"		||
			name == "output_tooltips"	||
			name == "experimental"		||
			name == "deprecated"		||
			name == "api_node"
			)
		{
			continue;
		}

		LogUnhandledJson("/unhandled node's attr. Node=" << node.name << ", attr=" << name);
	}
}

void NodesParser::add_internal_nodes(NodesManager& manager)
{
	// Reroute
	{
		auto node = std::make_shared<data::Node>();
		node->name = "Reroute";
		node->display_name = "Reroute";
		node->category = "utils";
		node->description = "Reroute Node Node Purpose: Mainly used to organize the logic of overly long connection lines in the ComfyUI workflow.";

		// Input: ANY (*)
		auto in = std::make_shared<data::Pin>();
		in->name = "*";
		in->input = true;
		in->initialize("*");
		node->required.pins.push_back(in);
		node->required.order.push_back("*");

		// Output: ANY (*)
		auto out = std::make_shared<data::Pin>();
		out->name = "*";
		out->output = true;
		in->initialize("*");
		node->output.push_back(out);

		node->has_input = true;
		node->has_output = true;

		put_in_tree(manager, node);
		manager.m_nodes_by_id.emplace(node->get_id(), node);
	}
	
	// Primitive
	{
		auto node = std::make_shared<data::Node>();
		node->name = "Primitive";
		node->display_name = "Primitive";
		node->category = "utils";
		node->description = "Primitive values like numbers, strings, and booleans.";

		auto out = std::make_shared<data::Pin>();
		out->name = "connect to widget input";
		out->output = true;
		out->initialize("*");

		node->output.push_back(out);
		node->has_output = true;

		put_in_tree(manager, node);
		manager.m_nodes_by_id.emplace(node->get_id(), node);
	}

	// Note
	{
		auto node = std::make_shared<data::NodeNote>();
		node->name = "Note";
		node->display_name = "Note";
		node->category = "utils";
		node->description = "Node to add annotations to a workflow.";

		put_in_tree(manager, node);
		manager.m_nodes_by_id.emplace(node->get_id(), node);
	}

	// Markdown Note
	{
		auto node = std::make_shared<data::NodeMarkdownNote>();
		node->name = "Markdown Note";
		node->display_name = "Markdown Note";
		node->category = "utils";
		node->description = "Node to add annotations to a workflow. It supports text formatting using Markdown syntax.";

		put_in_tree(manager, node);
		manager.m_nodes_by_id.emplace(node->get_id(), node);
	}
}

void NodesParser::put_in_tree(NodesManager& manager, std::shared_ptr<data::Node> node)
{
	const auto search_branch = [](std::vector<data::Nodes>* items, const std::string& category)
	{
		for (auto it = items->begin(); it != items->end(); ++it)
		{
			if (!it->is_node && category == it->name_or_category)
				return it;
		}
		return items->end();
	};

	const auto cats = tools::str::split(node->category, '/', false /*add_empty*/);
	std::vector<data::Nodes>* root = &manager.m_nodes_tree;

	auto cat = cats.begin();
	for ( ; cat != cats.end(); ++cat)
	{
		auto it = search_branch(root, *cat);
		if (it == root->end())
			break;
		root = &(*it).children;
	}

	for ( ; cat != cats.end(); ++cat)
	{
		root->push_back({});
		data::Nodes& branch = root->back();
		branch.is_node = false;
		branch.name_or_category = *cat;
		root = &branch.children;
	}

	root->push_back({});
	data::Nodes& branch = root->back();
	branch.is_node = true;
	branch.name_or_category = node->display_name;
	branch.node = node;

	manager.m_nodes_sorted[node->name] = node;
}

void NodesParser::sort_nodes_tree(NodesManager& manager)
{
	const auto sort_tree_callback = [](const data::Nodes& lhs, const data::Nodes& rhs)
	{
		return lhs.name_or_category < rhs.name_or_category;
	};

	const auto sort_plain_callback = [](const std::shared_ptr<data::Node>& lhs, const std::shared_ptr<data::Node>& rhs)
	{
		return lhs->display_name < rhs->display_name;
	};

	std::function<void(std::vector<data::Nodes>& nodes)> sort_tree;
	sort_tree = [&sort_tree, &sort_tree_callback](std::vector<data::Nodes>& nodes)
	{
		std::sort(nodes.begin(), nodes.end(), sort_tree_callback);

		for (auto& node : nodes)
		{
			sort_tree(node.children);
		}
	};

	sort_tree(manager.m_nodes_tree);
}

} // namespace mngr