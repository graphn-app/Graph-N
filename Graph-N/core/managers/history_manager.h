
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

// !! types order must remain the same due to serialization
using HActionT = std::variant<std::string /*rename*/, ImU32 /*color*/, NodeMode /*mode*/, bool /*pinning*/, ImRect /*pos*/, primitive /*pin val*/>;

enum class HistoryActionType
{
    _none,
    NodeAdd, NodeDel, NodeRen, NodeRect, NodePin, NodeMode, NodeClr, NodeExpand, 
    PinRen, PinVal,
    LinkAdd, LinkDel,

    _first = NodeAdd,
    _last = LinkDel
    // !! check serialization when adding new parameter
};

struct HistoryStep
{
    HistoryStep(HistoryActionType t);
    virtual ~HistoryStep();
    
    NO_COPY_MOVE(HistoryStep);

    HistoryActionType type = HistoryActionType::_none;
    iid id = 0;
};

struct HistoryAction final
{
    HistoryAction(int64_t id_, wid wf_);
    ~HistoryAction();

    HistoryAction(const HistoryAction&) = delete;
    HistoryAction& operator=(const HistoryAction&) = delete;
    HistoryAction(HistoryAction&& other);
    HistoryAction& operator=(HistoryAction&& other);

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
    ActNodeBlob(HistoryActionType t) : HistoryStep(t) { }
    ~ActNodeBlob() override = default;

    blob::Blob node;
    std::string debug_name;
};

struct ActOldNew : public HistoryStep
{
    ActOldNew(HistoryActionType t) : HistoryStep(t) { }
    ~ActOldNew() override = default;

    HActionT old_val;
    HActionT new_val;
};

struct ActLink : public HistoryStep
{
    ActLink(HistoryActionType t) : HistoryStep(t) { }
    ~ActLink() override = default;

    iid in = 0;
    iid out = 0;
};

////////// SPECIALIZATION       /////////////////////
/////////////////////////////////////////////////////

struct ActNodeAdd final : public ActNodeBlob { ActNodeAdd() : ActNodeBlob(HistoryActionType::NodeAdd) {} };
struct ActNodeDel final : public ActNodeBlob { ActNodeDel() : ActNodeBlob(HistoryActionType::NodeDel) {} };
struct ActNodeRen final : public ActOldNew   { ActNodeRen() : ActOldNew(HistoryActionType::NodeRen) {} };
struct ActNodeRect final : public ActOldNew  { ActNodeRect() : ActOldNew(HistoryActionType::NodeRect) {} };
struct ActNodePin final : public ActOldNew   { ActNodePin() : ActOldNew(HistoryActionType::NodePin) {} };
struct ActNodeMode final : public ActOldNew  { ActNodeMode() : ActOldNew(HistoryActionType::NodeMode) {} };
struct ActNodeClr final : public ActOldNew   { ActNodeClr() : ActOldNew(HistoryActionType::NodeClr) {} };
struct ActNodeExpand final : public ActOldNew   { ActNodeExpand() : ActOldNew(HistoryActionType::NodeExpand) {} };
struct ActPinRen final : public ActOldNew    { ActPinRen() : ActOldNew(HistoryActionType::PinRen) {} };
struct ActPinVal final : public ActOldNew    { ActPinVal() : ActOldNew(HistoryActionType::PinVal) {} };
struct ActLinkAdd final : public ActLink     { ActLinkAdd() : ActLink(HistoryActionType::LinkAdd) {} };
struct ActLinkDel final : public ActLink     { ActLinkDel() : ActLink(HistoryActionType::LinkDel) {} };

////////// HISTORY - main interface  /////////////////////
//////////////////////////////////////////////////////////

struct HistoryState
{
    int32_t pos = -1;
    int32_t saved_pos = -1;
    const std::string* undo_name = nullptr;
    const std::string* redo_name = nullptr;
};

class HistoryManager final : public Manager
{
public:
    HistoryManager();
    ~HistoryManager() override;

    bool init() override;
    void free() override;

    void stop_all_threads() override;

    void cleanup(wid wf_id);

    HistoryState get_state(wid wf_id) const;

    void undo(wid wf_id);
    void redo(wid wf_id);

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

    struct Meta
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
    void pop_node_clr(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_expand(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_ren(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_mode(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_pin(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_node_rect(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_pin_ren(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_pin_val(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_link_add(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);
    void pop_link_del(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo);

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
};

} // namespace mngr
