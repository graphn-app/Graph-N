#include "core/managers/history_manager.h"
#include "core/managers/workflows_manager.h"

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
    case HistoryActionType::NodeExpand: return "NodeExpand";
    case HistoryActionType::PinRen:  return "PinRen";
    case HistoryActionType::PinVal:  return "PinVal";
    case HistoryActionType::PinCag:  return "PinCag";
    case HistoryActionType::PinMultiline:  return "PinMultiline";
    case HistoryActionType::LinkAdd: return "LinkAdd";
    case HistoryActionType::LinkDel: return "LinkDel";
    case HistoryActionType::NodeRerouteSplit: return "RerouteSplit";
    }
    LogRErrorS("Unknown action type", "Unknown");
}

std::string prim_to_str(const Primitive& p)
{
    return
        std::visit([](const auto& v) -> std::string
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>)
                return "null";
            else if constexpr (std::is_same_v<T, std::string>)
                return fmt::format(FMT_STRING("str({} bytes)"), v.size());
            else if constexpr (std::is_same_v<T, number>)
                return fmt::format(FMT_STRING("{}"), v.underlying_type());
            else
                return fmt::format(FMT_STRING("{}"), v);
        }, p.value());
}

std::string action_val_to_str(const HActionT& val)
{
    return
        std::visit([](const auto& v) -> std::string
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>)
                return fmt::format(FMT_STRING("\"{}\""), v);
            else if constexpr (std::is_same_v<T, ImU32>)
                return fmt::format(FMT_STRING("#{:08X}"), v);
            else if constexpr (std::is_same_v<T, uint64_t>)
                return fmt::format(FMT_STRING("Flag({})"), cast::to_u64(v));
            else if constexpr (std::is_same_v<T, Primitive>)
                return prim_to_str(v);
            else if constexpr (std::is_same_v<T, ImRect>)
                return fmt::format(FMT_STRING("[{:.0f},{:.0f} {:.0f}x{:.0f}]"), v.Min.x, v.Min.y, v.GetWidth(), v.GetHeight());
            else if constexpr (std::is_same_v<T, ImVec4>)
                return fmt::format(FMT_STRING("({:.2f},{:.2f},{:.2f},{:.2f})"), v.x, v.y, v.z, v.w);
            else
                return "?";
        }, val);
}
} // namespace

void HistoryManager::LogAction(const HistoryAction& action, const char* state, int32_t pos /*= 0*/, int32_t saved_pos /*= 0*/, size_t total /*= 0*/) const
{
    const auto& wf_data = g::pWfsManager->access(action.wf)->get_data();
    const std::string pos_info = total > 0 ? fmt::format(FMT_STRING(" [Pos:{}/{}, Saved:{}]"), pos, total, saved_pos) : "";
    Log(fmt::format(FMT_STRING("[Wf:{}, {}, {}] Act #{} '{}'{} | Sts:{} Cntr:{} -> {}"), 
        action.wf, (wf_data.dirty ? "dirty" : "stable"), (wf_data.draft ? "draft" : "saved"),
        action.id, action.name, pos_info, 
        action.steps.size(), action.counter, state));
}

void HistoryManager::LogAction(wid wf_id, const char* state, int32_t pos /*= 0*/, int32_t saved_pos /*= 0*/, size_t total /*= 0*/) const
{
    const auto& wf_data = g::pWfsManager->access(wf_id)->get_data();
    Log(fmt::format(FMT_STRING("[Wf:{}, {}] [Pos:{}/{}, Saved:{}] -> {}"), 
        wf_id, (wf_data.dirty ? "dirty" : "stable"),
        pos, total, saved_pos,
        state));
}

void HistoryManager::LogStep(const HistoryStep* step, bool is_undo /*= false*/) const
{
    const char* prefix = is_undo ? "<- [UNDO]" : "-> [REDO]";
    std::string details;

    if (auto act_oldnew = cast::dynamic<const ActOldNew>(step))
    {
        const auto& v1 = is_undo ? act_oldnew->new_val : act_oldnew->old_val;
        const auto& v2 = is_undo ? act_oldnew->old_val : act_oldnew->new_val;
        details = fmt::format(FMT_STRING("{} => {}"), action_val_to_str(v1), action_val_to_str(v2));
    }
    else if (auto act_link = cast::dynamic<const ActLink>(step))
    {
        details = fmt::format(FMT_STRING("in:{} <-> out:{}"), act_link->pin_in, act_link->pin_out);
    }
    else if (auto act_blob = cast::dynamic<const ActNodeBlob>(step))
    {
        details = fmt::format(FMT_STRING("type: '{}', blob size: {} bytes"), act_blob->debug_name, act_blob->node->size());
    }

    Log(fmt::format(FMT_STRING("    {} Step [{}] iid:{} | {}"), prefix, action_type_str(step->type), step->id, details));
}

} // namespace mngr

