
#include "core/managers/history_manager.h"
#include "tools/blob_serializer.h"

namespace mngr
{

namespace
{

const char* action_type_str(HistoryActionType t)
{
    switch (t)
    {
    case HistoryActionType::NodeAdd: return "NodeAdd";
    case HistoryActionType::NodeDel: return "NodeDel";
    case HistoryActionType::NodeRen: return "NodeRen";
    case HistoryActionType::NodeRect: return "NodeRect";
    case HistoryActionType::NodePin: return "NodePin";
    case HistoryActionType::NodeMode: return "NodeMode";
    case HistoryActionType::NodeClr: return "NodeClr";
    case HistoryActionType::PinRen:  return "PinRen";
    case HistoryActionType::PinVal:  return "PinVal";
    case HistoryActionType::LinkAdd: return "LinkAdd";
    case HistoryActionType::LinkDel: return "LinkDel";
    default: return "Unknown";
    }
}

std::string prim_to_str(const primitive& p)
{
    return
        std::visit([](const auto& v) -> std::string
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>)
                return "null";
            else if constexpr (std::is_same_v<T, std::string>)
                return fmt::format("str({} bytes)", v.size());
            else
                return fmt::format("{}", v);
        }, p);
}

std::string action_val_to_str(const HActionT& val)
{
    return
        std::visit([](const auto& v) -> std::string
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>)
                return fmt::format("\"{}\"", v);
            else if constexpr (std::is_same_v<T, ImU32>)
                return fmt::format("#{:08X}", v);
            else if constexpr (std::is_same_v<T, bool>)
                return v ? "true" : "false";
            else if constexpr (std::is_same_v<T, int32_t>)
                return fmt::format("{}", v);
            else if constexpr (std::is_same_v<T, NodeMode>)
                return fmt::format("Mode({})", cast::to_i32(v));
            else if constexpr (std::is_same_v<T, primitive>)
                return prim_to_str(v);
            else if constexpr (std::is_same_v<T, ImRect>)
                return fmt::format("[{:.0f},{:.0f} {:.0f}x{:.0f}]", v.Min.x, v.Min.y, v.GetWidth(), v.GetHeight());
            else if constexpr (std::is_same_v<T, ImVec4>)
                return fmt::format("({:.2f},{:.2f},{:.2f},{:.2f})", v.x, v.y, v.z, v.w);
            else
                return "?";
        }, val);
}

} // namespace

void HistoryManager::LogAction(const HistoryAction& action, const char* state, size_t pos /*= 0*/, size_t total /*= 0*/)
{
    std::string pos_info = total > 0 ? fmt::format(" [Pos:{}/{}]", pos, total - 1) : "";
    Log(fmt::format("[Wf:{}] Act #{} '{}'{} | Sts:{} Cntr:{} -> {}", 
        action.wf, action.id, action.name, pos_info, 
        action.steps.size(), action.multi_action_counter, state));
}

void HistoryManager::LogStep(const HistoryStep* step, bool is_undo /*= false*/)
{
    const char* prefix = is_undo ? "<- [UNDO]" : "-> [REDO/DO]";
    std::string details;

    if (auto act_oldnew = cast::dynamic<const ActOldNew*>(step))
    {
        const auto& v1 = is_undo ? act_oldnew->new_val : act_oldnew->old_val;
        const auto& v2 = is_undo ? act_oldnew->old_val : act_oldnew->new_val;
        details = fmt::format("{} => {}", action_val_to_str(v1), action_val_to_str(v2));
    }
    else if (auto act_link = cast::dynamic<const ActLink*>(step))
    {
        details = fmt::format("in:{} <-> out:{}", act_link->in, act_link->out);
    }
    else if (auto act_blob = cast::dynamic<const ActNodeBlob*>(step))
    {
        details = fmt::format("type: '{}', blob size: {} bytes", act_blob->debug_name, act_blob->node->size());
    }

    Log(fmt::format("    {} Step [{}] iid:{} | {}", prefix, action_type_str(step->type), step->id, details));
}

void HistoryManager::serialize_action(blob::Writer& writer, const HActionT& val) const
{
    writer.push(cast::to_u8(val.index()));
    std::visit([&writer](const auto& v) 
    {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>)
        {
            writer.push(v);
        }
        else if constexpr (std::is_same_v<T, ImU32>)
        {
            writer.push(v);
        }
        else if constexpr (std::is_same_v<T, NodeMode>)
        {
            writer.push(cast::to_i32(v));
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            writer.push(v);
        }
        else if constexpr (std::is_same_v<T, ImRect>)
        {
            writer.push(v);
        }
        else if constexpr (std::is_same_v<T, primitive>)
        {
            writer.push(v);
        }
    }, val);
}

HActionT HistoryManager::deserialize_action(blob::Reader& reader)
{
    const auto idx = reader.pop_u8();
    switch (idx)
    {
    case 0:
        return std::string(reader.pop_string());
    case 1:
        return reader.pop_clr();
    case 2:
        return cast::to_enum<NodeMode>(reader.pop_i32());
    case 3:
        return reader.pop_bool();
    case 4:
        return reader.pop_rect();
    case 5:
        return reader.pop_primitive();
    default:
        {
            LogAndThrowInternalError(ExcepCategory::Logic, "Invalid HActionT variant index");
        }
    }
}

blob::Writer& HistoryManager::serialize(blob::Writer& writer) const
{
    writer.push(cast::to_u32(m_history.size()));
    for (auto& pair : m_history)
    {
        blob::Block block(writer);
        serialize(writer, pair.first);
    }
    return writer;
}

blob::Writer& HistoryManager::serialize(blob::Writer& writer, wid wf_id) const
{
    const auto it = m_history.find(wf_id);
    if (it == m_history.end())
    {
        writer.push(wf_id);
        writer.push(uint32_t(0)); // pos
        writer.push(uint32_t(0)); // history size
        return writer;
    }

    const auto& queue = it->second;
    
    writer.push(wf_id);
    const auto pos_it = m_pos.find(wf_id);
    writer.push(cast::to_u32(pos_it != m_pos.end() ? pos_it->second : 0));
    writer.push(cast::to_u32(queue.size()));

    for (const auto& act : queue)
    {
        writer.push(act.id);
        writer.push(act.name);
        writer.push(act.multi_action_counter);
        writer.push(cast::to_u32(act.steps.size()));

        for (const auto& step : act.steps)
        {
            writer.push(cast::to_u8(step->type));
            writer.push(step->id);

            switch (step->type)
            {
            case HistoryActionType::NodeAdd:
            case HistoryActionType::NodeDel:
                {
                    auto pstep = cast::dynamic<const ActNodeBlob*>(step.get());
                    writer.push(pstep->debug_name);
                    writer.push(pstep->node);
                }       
                break;
            case HistoryActionType::LinkAdd:
            case HistoryActionType::LinkDel:
                {
                    auto pstep = cast::dynamic<const ActLink*>(step.get());
                    writer.push(pstep->in);
                    writer.push(pstep->out);
                }       
                break;
            case HistoryActionType::NodeRen:
            case HistoryActionType::NodePin:
            case HistoryActionType::NodeRect:
            case HistoryActionType::NodeMode:
            case HistoryActionType::NodeClr:
            case HistoryActionType::PinRen:
            case HistoryActionType::PinVal:
                {
                    auto pstep = cast::dynamic<const ActOldNew*>(step.get());
                    serialize_action(writer, pstep->old_val);
                    serialize_action(writer, pstep->new_val);
                }       
                break;
            default:
                {
                    LogAndThrowInternalError(ExcepCategory::Logic, "Unhandled type");
                }
            }
        }
    }
    return writer;
}

bool HistoryManager::deserialize(blob::Reader& reader)
{
    try
    {
        const uint32_t h_count = reader.pop_u32();
        for (uint32_t h = 0; h < h_count; h++)
        {
            auto wf_reader = reader.pop_block();

            const wid wf_id = wf_reader.pop_wid();
            m_pos[wf_id] = wf_reader.pop_u32();
            const uint32_t queue_size = wf_reader.pop_u32();
            hqueue& queue = m_history[wf_id];
            queue.clear();

            for (uint32_t j = 0; j < queue_size; ++j)
            {
                HistoryAction act(wf_reader.pop_i64(), wf_id);
                act.name = wf_reader.pop_string();
                act.multi_action_counter = wf_reader.pop_i32();

                const uint32_t steps_count = wf_reader.pop_u32();
                for (uint32_t k = 0; k < steps_count; ++k)
                {
                    const HistoryActionType type = cast::to_enum<HistoryActionType>(wf_reader.pop_u8());
                    const iid id = wf_reader.pop_iid();

                    std::unique_ptr<HistoryStep> step;

                    switch (type)
                    {
                    case HistoryActionType::NodeAdd:
                    case HistoryActionType::NodeDel:
                    {
                        std::unique_ptr<ActNodeBlob> blob_step;
                        if (HistoryActionType::NodeAdd == type)
                            blob_step = std::make_unique<ActNodeAdd>();
                        else
                            blob_step = std::make_unique<ActNodeDel>();

                        blob_step->debug_name = wf_reader.pop_string();
                        blob_step->node = wf_reader.pop_blob();
                        step = std::move(blob_step);
                    }
                    break;
                    case HistoryActionType::LinkAdd:
                    case HistoryActionType::LinkDel:
                    {
                        std::unique_ptr<ActLink> blob_step;
                        if (HistoryActionType::LinkAdd == type)
                            blob_step = std::make_unique<ActLinkAdd>();
                        else
                            blob_step = std::make_unique<ActLinkDel>();

                        blob_step->in = wf_reader.pop_iid();
                        blob_step->out = wf_reader.pop_iid();
                        step = std::move(blob_step);
                    }
                    break;
                    default:
                    {
                        std::unique_ptr<ActOldNew> blob_step;
                        switch (type)
                        {
                        case HistoryActionType::NodeRen:  blob_step = std::make_unique<ActNodeRen>(); break;
                        case HistoryActionType::NodeRect: blob_step = std::make_unique<ActNodeRect>(); break;
                        case HistoryActionType::NodePin:  blob_step = std::make_unique<ActNodePin>(); break;
                        case HistoryActionType::NodeMode: blob_step = std::make_unique<ActNodeMode>(); break;
                        case HistoryActionType::NodeClr:  blob_step = std::make_unique<ActNodeClr>(); break;
                        case HistoryActionType::PinRen:   blob_step = std::make_unique<ActPinRen>(); break;
                        case HistoryActionType::PinVal:   blob_step = std::make_unique<ActPinVal>(); break;
                        default: { LogAndThrowInternalError(ExcepCategory::Logic, "Unhandled type"); }
                        }
                        blob_step->old_val = deserialize_action(wf_reader);
                        blob_step->new_val = deserialize_action(wf_reader);
                        step = std::move(blob_step);
                    }
                    }

                    step->id = id;
                    act.steps.push_back(std::move(step));
                }
                queue.push_back(std::move(act));
            }
        }
        return true;
    }
    catch (std::exception& ex)
    {
        LogExcep(ExcepCategory::Logic, ex.what());
    }
    return false;
}

} // namespace mngr

