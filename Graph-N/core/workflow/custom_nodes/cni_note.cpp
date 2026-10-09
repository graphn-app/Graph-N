#include "core/workflow/custom_nodes/cni_note.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"

namespace data
{

template <typename T>
CNI_NoteBase<T>::CNI_NoteBase(Node* obj, wid workflow_id, iid instance_id) noexcept
    : CNI_BaseObj_T<T>(obj, workflow_id, instance_id)
{
    this->flags().add(NodeInstFlags::CNI, NodeInstFlags::JsonApiDontExport, NodeInstFlags::BlobCustomSerialization);
}

template <typename T>
void CNI_NoteBase<T>::cni_cleanup_base_object() noexcept
{
    auto& ins = this->get_inputs();
    for (auto in : ins)
    {
        this->cni_remove_pin(in->get_object());
    }
}

template <typename T>
void CNI_NoteBase<T>::cni_init()
{
    auto wf = g::pWfsManager->access(this->get_workflow_id());
    auto& inputs = this->get_inputs();
    for (auto input : inputs)
    {
        this->cni_remove_pin(input->get_object());
        this->remove_pin(input);
        wf->remove_pin(input);
    }
    this->reset_pins();

    iid id = 0;
    Primitive value = std::string();

    CNI_DeserializedNote* deserialized_values = nullptr;
    if (this->m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedNote>(this->m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");

        if (deserialized_values != nullptr)
        {
            id = deserialized_values->id;
            value = std::move(deserialized_values->widget_value);
        }
    }
    this->m_deserialized_values.reset();

    auto pin = g::pNodesManager->make_pin_ptr(this->get_object(), "STRING", "", -1);
    pin->placeholder = "Put note here";
    pin->flags.add(PinObjFlags::Multiline, PinObjFlags::DenyMultilineSwitch);
    pin->flags.remove(PinObjFlags::CanRename, PinObjFlags::CanLink, PinObjFlags::CanUnlink);
    auto obj = pin.get();
    this->cni_add_pin(std::move(pin));

    auto instance = wf->add_pin(this, obj, id);
    instance->state().value = std::move(value);
    this->add_input(instance);
}

template <typename T>
bool CNI_NoteBase<T>::cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    auto& src_ins = copy_from_node->get_inputs();
    LogRAssert(!src_ins.empty(), "Source node pins empty", true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedNote>();
    auto note_data = cast::strict<CNI_DeserializedNote>(data.get());
    if (copy_pin_ids)
        note_data->id = src_ins[0]->get_instance_id();
    note_data->widget_value = src_ins[0]->state().value;

    this->cni_save_deserialized_info(std::move(data));
    return true;
}

template class CNI_NoteBase<CNI_Note>;
template class CNI_NoteBase<CNI_MarkdownNote>;


std::unique_ptr<Node> CNI_Note::cni_create_object()
{
    auto node = std::make_unique<data::Node>(js::NOTE_CLASS);
    node->name = node->display_name = "Note";
    node->category = "utils";
    node->description = "Node to add annotations to a workflow.";
    node->python_module = VANILLA;
    node->python_module_hash = base::HashID::make_hash(node->python_module);
    node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
    node->flags.add(NodeObjFlags::SpecNote, NodeObjFlags::RestrictCfg);
    node->finalize(nullptr);
    return node;
}

std::unique_ptr<Node> CNI_MarkdownNote::cni_create_object()
{
    auto node = std::make_unique<data::Node>(js::MARKDOWN_NOTE_CLASS);
    node->name = node->display_name = "Markdown Note";
    node->category = "utils";
    node->description = "Node to add annotations to a workflow. It supports text formatting using Markdown syntax.";
    node->python_module = VANILLA;
    node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanMute);
    node->flags.add(NodeObjFlags::SpecMdNote, NodeObjFlags::RestrictCfg);
    node->finalize(nullptr);
    return node;
}

} // namespace data
