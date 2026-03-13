
#pragma once
#include "core/workflow/instance.h"

namespace data
{
struct Pin;
struct PinState;

class PinInstance final : public Instance<Pin>
{
public:
    PinInstance(iid parent_node, Pin* obj, wid workflow_id, iid instance_id);
    ~PinInstance() override;

    void init();

    inline iid get_parent_node() const { return m_parent_node; }

    inline PinState& state() { return m_state; }
    inline const PinState& state() const { return m_state; }

    blob::Writer& serialize(blob::Writer& writer) const;
    blob::Writer serialize() const;
    bool deserialize(blob::Reader& reader);

    blob::Writer serialize_state() const;
    blob::Writer serialize_state(const primitive& val) const;
    blob::Writer& serialize_state(blob::Writer& writer) const;
    blob::Writer& serialize_state(blob::Writer& writer, const primitive& val) const;

    bool deserialize_state(blob::Reader& reader);

protected:
    data::Pin* find_obj(hid obj_id) const override;

private:
    PinState m_state;
    iid m_parent_node = 0;
};

} // namespace data
