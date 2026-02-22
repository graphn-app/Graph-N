
#pragma once
#include "core/workflow/node_instance_base.h"

namespace data
{

class PinInstance;
struct Node;

class NodeInstanceRo final : public NodeInstanceBase
{
public:
    using NodeInstanceBase::NodeInstanceBase;
    ~NodeInstanceRo() override;

    void init() override;
};

} // namespace data

