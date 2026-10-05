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
    o_json serialize_custom_node(data::NodeInstance* node, int32_t& input_count);

private:
    const Workflow* m_wf = nullptr;
};

} // namespace data
