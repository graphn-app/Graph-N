/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/workflow/node_instance_base.h"

namespace data
{

class PinInstance;
struct Node;

class NodeInstanceRo : public NodeInstanceBase
{
public:
    using NodeInstanceBase::NodeInstanceBase;
    ~NodeInstanceRo() noexcept override;

    void init() override;
};

} // namespace data

