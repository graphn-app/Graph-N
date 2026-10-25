/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace data
{
class Workflow;
class NodeInstance;

class WfSerializerToJsonApi final
{
public:
    WfSerializerToJsonApi(const Workflow* wf) noexcept;
    ~WfSerializerToJsonApi() noexcept = default;

    NO_COPY_MOVE(WfSerializerToJsonApi);

    o_json serialize();

private:
    void serialize_custom_node(data::NodeInstance* node, o_json& jinputs);

private:
    const Workflow* m_wf = nullptr;
};

} // namespace data
