#pragma once

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;

class WfSerializerToJsonApi final
{
public:
    WfSerializerToJsonApi(const Workflow* wf);
    ~WfSerializerToJsonApi() = default;

    NO_COPY_MOVE(WfSerializerToJsonApi);

    o_json serialize();

private:
    const Workflow* m_wf = nullptr;
};

} // namespace data
