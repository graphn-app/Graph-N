#pragma once
#include "core/managers/manager.h"

namespace data
{
class NodeInstance;
class PinInstance;
class Link;
class Workflow;
}

////////// TYPE                 /////////////////////
/////////////////////////////////////////////////////

namespace mngr
{

struct HRerouteSplitted final
{
    std::pair<ImRect, ImRect> poses;
    bool splitted = true;
};
struct HPinCag final
{
    ControlAfterGenerate cag;
    Primitive min;
    Primitive max;
    Primitive step;
};

// !! types order must remain the same due to serialization
using HActionT = std::variant<std::string /*rename*/, ImU32 /*color*/, uint64_t /*flags*/, ImRect /*pos*/, Primitive /*pin val*/, HRerouteSplitted, HPinCag>;

enum class HistoryActionType
{
    _none,
    NodeAdd, NodeDel, NodeRen, NodeRect, NodePin, NodeMode, NodeClr, NodeExpand, NodeRerouteSplit,
    PinRen, PinVal, PinCag, PinMultiline,
    LinkAdd, LinkDel,

    _first = NodeAdd,
    _last = LinkDel
    // !! check serialization when adding new parameter
};

struct HistoryStep
{
    HistoryStep(HistoryActionType t) noexcept;
    virtual ~HistoryStep() noexcept;
    
    NO_COPY_MOVE(HistoryStep);

    HistoryActionType type = HistoryActionType::_none;
    iid id = 0;
};

struct HistoryAction final
{
    HistoryAction(int64_t id_, wid wf_) noexcept;
    ~HistoryAction() noexcept;

    HistoryAction(const HistoryAction&) = delete;
    HistoryAction& operator=(const HistoryAction&) = delete;
    HistoryAction(HistoryAction&& other) noexcept;
    HistoryAction& operator=(HistoryAction&& other) noexcept;

    int64_t id = 0;
    wid wf = 0;
    std::string name;
    int32_t counter = 0;
    std::list<std::unique_ptr<HistoryStep>> steps;
    //!!! check serializer when adding new member
};

////////// BASE ACTION          /////////////////////
/////////////////////////////////////////////////////

struct ActNodeBlob : public HistoryStep
{
    ActNodeBlob(HistoryActionType t) noexcept : HistoryStep(t) { }
    ~ActNodeBlob() noexcept override = default;

    blob::Blob node;
    std::string debug_name;
};

struct ActOldNew : public HistoryStep
{
    ActOldNew(HistoryActionType t) noexcept : HistoryStep(t) { }
    ~ActOldNew() noexcept override = default;

    HActionT old_val;
    HActionT new_val;
};

struct ActLink : public HistoryStep
{
    ActLink(HistoryActionType t) noexcept : HistoryStep(t) { }
    ~ActLink() noexcept override = default;

    iid node_in = 0;
    iid pin_in = 0;
    std::string pin_in_name;
    iid node_out = 0;
    iid pin_out = 0;
    int32_t pin_out_index = 0;
    std::string linked_pins_type;
};

////////// SPECIALIZATION       /////////////////////
/////////////////////////////////////////////////////

struct ActNodeAdd final : public ActNodeBlob { ActNodeAdd() noexcept : ActNodeBlob(HistoryActionType::NodeAdd) {} };
struct ActNodeDel final : public ActNodeBlob { ActNodeDel() noexcept : ActNodeBlob(HistoryActionType::NodeDel) {} };
struct ActNodeRen final : public ActOldNew   { ActNodeRen() noexcept : ActOldNew(HistoryActionType::NodeRen) {} };
struct ActNodeRect final : public ActOldNew  { ActNodeRect() noexcept : ActOldNew(HistoryActionType::NodeRect) {} };
struct ActNodePin final : public ActOldNew   { ActNodePin() noexcept : ActOldNew(HistoryActionType::NodePin) {} };
struct ActNodeMode final : public ActOldNew  { ActNodeMode() noexcept : ActOldNew(HistoryActionType::NodeMode) {} };
struct ActNodeClr final : public ActOldNew   { ActNodeClr() noexcept : ActOldNew(HistoryActionType::NodeClr) {} };
struct ActNodeExpand final : public ActOldNew   { ActNodeExpand() noexcept : ActOldNew(HistoryActionType::NodeExpand) {} };
struct ActNodeRerouteSplit final : public ActOldNew   { ActNodeRerouteSplit() noexcept : ActOldNew(HistoryActionType::NodeRerouteSplit) {} };
struct ActPinRen final : public ActOldNew    { ActPinRen() noexcept : ActOldNew(HistoryActionType::PinRen) {} };
struct ActPinVal final : public ActOldNew    { ActPinVal() noexcept : ActOldNew(HistoryActionType::PinVal) {} };
struct ActPinCag final : public ActOldNew    { ActPinCag() noexcept : ActOldNew(HistoryActionType::PinCag) {} };
struct ActPinMultiline final : public ActOldNew    { ActPinMultiline() noexcept : ActOldNew(HistoryActionType::PinMultiline) {} };
struct ActLinkAdd final : public ActLink     { ActLinkAdd() noexcept : ActLink(HistoryActionType::LinkAdd) {} };
struct ActLinkDel final : public ActLink     { ActLinkDel() noexcept : ActLink(HistoryActionType::LinkDel) {} };

////////// HISTORY - main interface  /////////////////////
//////////////////////////////////////////////////////////

struct HistoryState final
{
    int32_t pos = -1;
    int32_t saved_pos = -1;
    const std::string* undo_name = nullptr;
    const std::string* redo_name = nullptr;
};

class HistoryManager final : public Manager
{
public:
    HistoryManager() noexcept;
    ~HistoryManager() noexcept override;

    void cleanup(wid wf_id);

    HistoryState get_state(wid wf_id) const;

    void undo(wid wf_id);
    void redo(wid wf_id);

    void set_ignore_actions_on(wid wf_id);
    void set_ignore_actions_off(wid wf_id) noexcept;

    void set_rollback_pos(wid wf_id);
    bool can_rollback(wid wf_id);
    void rollback(wid wf_id);

    void multi_action_begin(wid wf_id, const char* action_name);
    void multi_action_end(wid wf_id);

    void push_node_action(const data::NodeInstance* node, HistoryActionType act); // add/delete only
    void push_node_action(const data::NodeInstance* node, HistoryActionType act, HActionT&& old_val);
    void push_pin_action(const data::PinInstance* pin, HistoryActionType act, HActionT&& old_val);
    void push_link_action(const data::Link* link, HistoryActionType act); // add/delete only

private:
    HistoryAction* open_action(wid wf_id, const char* action_name);
    void close_action(wid wf_id);

    bool should_ignore_action(wid wf_id);

    struct Meta final
    {
        int32_t pos = -1;
        int32_t saved = -2;
    };

    std::pair<bool, bool> get_undo_redo(wid wf_id, const HistoryAction** undo_h, const HistoryAction** redo_h, const Meta** meta) const;
    const HistoryAction* get_undo(wid wf_id, const Meta** meta) const;
    const HistoryAction* get_redo(wid wf_id, const Meta** meta) const;

    void sync_wf_state(wid wf_id, const Meta* meta) const;

    void pop_action(const HistoryAction* action, bool do_undo);
    void pop_action(data::Workflow* wf, const HistoryStep* step, bool do_undo);

    void pop_node_add(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_del(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_clr(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_expand(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_ren(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_mode(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_pin(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_rect(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_reroute_split(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_pin_ren(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_pin_val(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_pin_cag(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_pin_multiline(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_link_add(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_link_del(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);

    void LogAction(const HistoryAction& action, const char* state, int32_t pos = 0, int32_t saved_pos = 0, size_t total = 0) const;
    void LogAction(wid wf_id, const char* state, int32_t pos = 0, int32_t saved_pos = 0, size_t total = 0) const;
    void LogStep(const HistoryStep* step, bool is_undo = false) const;

private:
    const size_t m_limit;
    bool m_rollback = false;

    using hqueue = std::deque<HistoryAction>;

    int64_t m_last_id = 0;
    std::unordered_map<wid, Meta> m_meta;
    std::unordered_map<wid, hqueue> m_history;
    std::set<wid> m_ignore_actions;
};

} // namespace mngr
