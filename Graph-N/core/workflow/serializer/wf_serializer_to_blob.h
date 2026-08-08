#pragma once
#include "core/base/structs.h"

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;

class WfSerializerToBlob final
{
public:
    WfSerializerToBlob(const Workflow* wf) noexcept;
    ~WfSerializerToBlob() noexcept = default;

    NO_COPY_MOVE(WfSerializerToBlob);

    blob::Writer serialize(const blob::Blob& drawer_state);
    blob::Blob serialize_node(const NodeInstance* node);

private:
    void serialize_node();
    void serialize_pin();
    void serialize_link();

    void serialize_cni_node();
    void serialize_cni_node_primitive();
    void serialize_cni_node_reroute();
    void serialize_cni_node_note();
    void serialize_cni_node_markdownnote();

private:
    const Workflow* m_wf = nullptr;
    blob::Writer m_writer;
    const NodeInstance* m_curr_node = nullptr;
    const PinInstance* m_curr_pin = nullptr;
    const Link* m_curr_link = nullptr;
};

} // namespace data
