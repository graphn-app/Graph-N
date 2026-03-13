
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

blob::Writer& PinInstance::serialize(blob::Writer& blob) const
{
    blob
        .push(m_hid)
        .push(m_iid)
        .push(m_wid)
        .push(m_parent_node)
    ;
    serialize_state(blob);
    return blob;
}

blob::Writer PinInstance::serialize() const
{
    blob::Writer blob;
    serialize(blob);
    return blob;
}

bool PinInstance::deserialize(blob::Reader& blob)
{
    try
    {
        m_hid = blob.pop_hid();
        m_object = g::pNodesManager->find_pin(m_hid);
        if (nullptr == m_object)
        {
            LogWarn("Pin " << m_hid << " not found");
            return false;
        }
        m_state = m_object->def_state;

        m_iid = blob.pop_iid();
        m_wid = blob.pop_wid();
        m_parent_node = blob.pop_iid();
        return deserialize_state(blob);
    }
    catch (std::exception& ex)
    {
        LogExcep(ExcepCategory::Logic, ex.what());
        return false;
    }
}

blob::Writer PinInstance::serialize_state() const
{
    return serialize_state(m_state.value);
}

blob::Writer PinInstance::serialize_state(const primitive& val) const
{
    blob::Writer blob;
    serialize_state(blob, val);
    return blob;
}

blob::Writer& PinInstance::serialize_state(blob::Writer& blob) const
{
    return serialize_state(blob, m_state.value);
}

blob::Writer& PinInstance::serialize_state(blob::Writer& blob, const primitive& val) const
{
    blob
        .push(m_state.name)
        .push(val)
        .push(m_state.cag)
    ;
    return blob;
}

bool PinInstance::deserialize_state(blob::Reader& blob)
{
    m_state.name = blob.pop_string();
    m_state.value = blob.pop_primitive();
    m_state.cag = blob.pop_iid();
    return true;
}

} // namespace data
