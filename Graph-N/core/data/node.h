
#pragma once
#include "core/base/id.h"
#include "core/data/pin.h"

// todo: check utils

namespace data
{

struct Node : public base::LocalID
{
	Node() = default;
	virtual ~Node() override = default;

	bool has_input = false;
	struct Input
	{
		std::vector<std::shared_ptr<Pin>> pins;
		std::vector<std::string> order;
	};
	Input required;
	Input optional;
	Input hidden;	

	bool has_output = false;
	bool output_node = false;
	std::vector<std::shared_ptr<data::Pin>> output;

	std::string name;
	std::string node_class;		// if empty - matches to name
	std::string display_name;	// use name if empty
	std::string description;
	std::string category;		// full path

	std::string python_module;
	bool experimental = false;
	bool deprecated = false;
	bool api_node = false;
};

} // namespace data