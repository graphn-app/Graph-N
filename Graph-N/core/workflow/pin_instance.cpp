
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/managers/nodes_manager.h"
#include "core/data/pin.h"

namespace data
{

PinInstance::PinInstance(iid parent_node, Pin* obj, wid workflow_id, iid instance_id)
    : Instance<Pin>(obj, workflow_id, instance_id)
    , m_parent_node(parent_node)
{}

PinInstance::~PinInstance() = default;

void PinInstance::init()
{
    Instance<data::Pin>::init();
    m_state = m_object->def_state;
}

data::Pin* PinInstance::find_obj(hid obj_id) const
{
    return g::pNodesManager->find_pin(obj_id);
}

blob::Writer& PinInstance::serialize(blob::Writer& writer) const
{
    writer
        .push(m_hid)
        .push(m_iid)
        .push(m_wid)
        .push(m_parent_node)
    ;
    serialize_state(writer);
    return writer;
}

blob::Writer PinInstance::serialize() const
{
    blob::Writer writer;
    serialize(writer);
    return writer;
}

bool PinInstance::deserialize(blob::Reader& reader)
{
    try
    {
        m_hid = reader.pop_hid();
        m_object = g::pNodesManager->find_pin(m_hid);
        if (nullptr == m_object)
        {
            LogWarn("Pin " << m_hid << " not found");
            return false;
        }
        m_state = m_object->def_state;

        m_iid = reader.pop_iid();
        m_wid = reader.pop_wid();
        m_parent_node = reader.pop_iid();
        return deserialize_state(reader);
    }
    catch (std::exception& ex)
    {
        LogExcep(ExcepCategory::Logic, ex.what());
    }
    return false;
}

blob::Writer PinInstance::serialize_state() const
{
    return serialize_state(m_state.value);
}

blob::Writer PinInstance::serialize_state(const primitive& val) const
{
    blob::Writer writer;
    serialize_state(writer, val);
    return writer;
}

blob::Writer& PinInstance::serialize_state(blob::Writer& writer) const
{
    return serialize_state(writer, m_state.value);
}

blob::Writer& PinInstance::serialize_state(blob::Writer& writer, const primitive& val) const
{
    writer
        .push(m_state.name)
        .push(val)
        .push(m_state.cag)
    ;
    return writer;
}

bool PinInstance::deserialize_state(blob::Reader& reader)
{
    try
    {
        m_state.name = reader.pop_string();
        m_state.value = reader.pop_primitive();
        m_state.cag = reader.pop_iid();
        return true;
    }
    catch (std::exception& ex)
    {
        LogExcep(ExcepCategory::Logic, ex.what());
    }
    return false;
}

} // namespace data
