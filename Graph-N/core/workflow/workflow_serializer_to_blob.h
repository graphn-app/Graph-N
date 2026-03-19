
#pragma once

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;

class WorkflowSerializerToBlob
{
    NO_COPY_MOVE_CTORS(WorkflowSerializerToBlob);
public:

    static blob::Writer serialize(const Workflow* wf, const blob::Blob& drawer_state);
    static bool deserialize(Workflow* wf, const blob::Blob& blob, blob::Blob& drawer_state);

    static blob::Blob serialize_node(const NodeInstance* node);
    static NodeInstance* deserialize_node(Workflow* wf, blob::Reader& reader);

private:
    static void serialize_node(const NodeInstance* node, blob::Writer& writer);
    static void serialize_pin(const PinInstance* pin, blob::Writer& writer);
    static void serialize_link(const Link* link, blob::Writer& writer);
    static void deserialize_node(Workflow* wf, NodeInstance* node, blob::Reader& reader);
    static void deserialize_pin(PinInstance* pin, blob::Reader& reader);
    static void deserialize_link(Workflow* wf, blob::Reader& reader);
};

} // namespace data
