//------------------------------------------------------------------------------
// VERSION 0.9.1
//
// LICENSE
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
//
// CREDITS
//   Written by Michal Cichon
//------------------------------------------------------------------------------
# include "imgui_node_editor_internal.h"
# include <algorithm>


//------------------------------------------------------------------------------
static ax::NodeEditor::Detail::EditorContext* s_Editor = nullptr;


//------------------------------------------------------------------------------
template <typename C, typename I, typename F>
static int BuildIdList(C& container, I* list, int listSize, F&& accept)
{
    if (list != nullptr)
    {
        int count = 0;
        for (auto object : container)
        {
            if (listSize <= 0)
                break;

            if (accept(object))
            {
                list[count] = I(object->ID().AsPointer());
                ++count;
                --listSize;
            }
        }

        return count;
    }
    else
        return static_cast<int>(std::count_if(container.begin(), container.end(), accept));
}


//------------------------------------------------------------------------------
ax::NodeEditor::EditorContext* ax::NodeEditor::CreateEditor(const Config* config)
{
    return reinterpret_cast<ax::NodeEditor::EditorContext*>(new ax::NodeEditor::Detail::EditorContext(config));
}

void ax::NodeEditor::DestroyEditor(EditorContext* ctx)
{
    auto lastContext = GetCurrentEditor();

    // Set context we're about to destroy as current, to give callback valid context
    if (lastContext != ctx)
        SetCurrentEditor(ctx);

    auto editor = reinterpret_cast<ax::NodeEditor::Detail::EditorContext*>(ctx);

    delete editor;

    if (lastContext != ctx)
        SetCurrentEditor(lastContext);
}

ax::NodeEditor::Config& ax::NodeEditor::EditConfig(EditorContext* ctx)
{
    if (ctx == nullptr)
        ctx = GetCurrentEditor();

    if (ctx)
    {
        auto editor = reinterpret_cast<ax::NodeEditor::Detail::EditorContext*>(ctx);

        return editor->GetConfig();
    }
    else
    {
        static Config s_EmptyConfig;
        return s_EmptyConfig;
    }
}

const ax::NodeEditor::Config& ax::NodeEditor::GetConfig(EditorContext* ctx)
{
    return EditConfig(ctx);
}

void ax::NodeEditor::SetCurrentEditor(EditorContext* ctx)
{
    s_Editor = reinterpret_cast<ax::NodeEditor::Detail::EditorContext*>(ctx);
}

ax::NodeEditor::EditorContext* ax::NodeEditor::GetCurrentEditor()
{
    return reinterpret_cast<ax::NodeEditor::EditorContext*>(s_Editor);
}

ax::NodeEditor::Style& ax::NodeEditor::GetStyle()
{
    return s_Editor->GetStyle();
}

const char* ax::NodeEditor::GetStyleColorName(StyleColor colorIndex)
{
    return s_Editor->GetStyle().GetColorName(colorIndex);
}

void ax::NodeEditor::PushStyleColor(StyleColor colorIndex, const ImVec4& color)
{
    s_Editor->GetStyle().PushColor(colorIndex, color);
}

void ax::NodeEditor::PopStyleColor(int count)
{
    s_Editor->GetStyle().PopColor(count);
}

void ax::NodeEditor::PushStyleVar(StyleVar varIndex, float value)
{
    s_Editor->GetStyle().PushVar(varIndex, value);
}

void ax::NodeEditor::PushStyleVar(StyleVar varIndex, const ImVec2& value)
{
    s_Editor->GetStyle().PushVar(varIndex, value);
}

void ax::NodeEditor::PushStyleVar(StyleVar varIndex, const ImVec4& value)
{
    s_Editor->GetStyle().PushVar(varIndex, value);
}

void ax::NodeEditor::PopStyleVar(int count)
{
    s_Editor->GetStyle().PopVar(count);
}

void ax::NodeEditor::Begin(const char* id, const ImVec2& size)
{
    s_Editor->Begin(id, size);
}

void ax::NodeEditor::End()
{
    s_Editor->End();
}

void ax::NodeEditor::BeginNode(EdType& edNode)
{
    if (edNode.has_object())
        s_Editor->GetNodeBuilder().Begin(edNode.get_node());
    else
        edNode.init(s_Editor->GetNodeBuilder().Begin(edNode.id()));
}

void ax::NodeEditor::BeginPin(EdType& edPin, PinKind kind)
{
    if (edPin.has_object())
        s_Editor->GetNodeBuilder().BeginPin(edPin.get_pin(), kind);
    else
        edPin.init(s_Editor->GetNodeBuilder().BeginPin(edPin.id(), kind));
}

void ax::NodeEditor::PinRect(const ImVec2& a, const ImVec2& b)
{
    s_Editor->GetNodeBuilder().PinRect(a, b);
}

void ax::NodeEditor::PinPivotRect(const ImVec2& a, const ImVec2& b)
{
    s_Editor->GetNodeBuilder().PinPivotRect(a, b);
}

void ax::NodeEditor::PinPivotSize(const ImVec2& size)
{
    s_Editor->GetNodeBuilder().PinPivotSize(size);
}

void ax::NodeEditor::PinPivotScale(const ImVec2& scale)
{
    s_Editor->GetNodeBuilder().PinPivotScale(scale);
}

void ax::NodeEditor::PinPivotAlignment(const ImVec2& alignment)
{
    s_Editor->GetNodeBuilder().PinPivotAlignment(alignment);
}

void ax::NodeEditor::EndPin()
{
    s_Editor->GetNodeBuilder().EndPin();
}

void ax::NodeEditor::Group(const ImVec2& size)
{
    s_Editor->GetNodeBuilder().Group(size);
}

void ax::NodeEditor::EndNode()
{
    s_Editor->GetNodeBuilder().End();
}

bool ax::NodeEditor::BeginGroupHint(NodeId nodeId)
{
    return s_Editor->GetHintBuilder().Begin(nodeId);
}

ImVec2 ax::NodeEditor::GetGroupMin()
{
    return s_Editor->GetHintBuilder().GetGroupMin();
}

ImVec2 ax::NodeEditor::GetGroupMax()
{
    return s_Editor->GetHintBuilder().GetGroupMax();
}

ImDrawList* ax::NodeEditor::GetHintForegroundDrawList()
{
    return s_Editor->GetHintBuilder().GetForegroundDrawList();
}

ImDrawList* ax::NodeEditor::GetHintBackgroundDrawList()
{
    return s_Editor->GetHintBuilder().GetBackgroundDrawList();
}

void ax::NodeEditor::EndGroupHint()
{
    s_Editor->GetHintBuilder().End();
}

ImDrawList* ax::NodeEditor::GetNodeBackgroundDrawList(NodeId nodeId)
{
    if (auto node = s_Editor->FindNode(nodeId))
        return s_Editor->GetNodeBuilder().GetUserBackgroundDrawList(node);
    else
        return nullptr;
}

void ax::NodeEditor::Link(EdType& edLink, const EdType& edStartPin, const EdType& edEndPin, ImU32 color, float thickness)
{
    IM_ASSERT(edStartPin.has_object());
    IM_ASSERT(edEndPin.has_object());

    if (edLink.has_object())
        s_Editor->DoLink(edLink.get_link(), edStartPin.get_pin(), edEndPin.get_pin(), color, thickness);
    else
        edLink.init(s_Editor->DoLink(edLink.id(), edStartPin.get_pin(), edEndPin.get_pin(), color, thickness));
}

void ax::NodeEditor::Flow(LinkId linkId, FlowDirection direction)
{
    if (auto link = s_Editor->FindLink(linkId))
        s_Editor->Flow(link, direction);
}

bool ax::NodeEditor::BeginCreate(const ImVec4& color, float thickness)
{
    auto& context = s_Editor->GetItemCreator();

    if (context.Begin())
    {
        context.SetStyle(ImColor(color), thickness);
        return true;
    }
    else
        return false;
}

bool ax::NodeEditor::QueryNewLink(uint64_t& startId, uint64_t& endId)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    startId = endId = 0;
    PinId pStartId, pEndId;
    if (context.QueryLink(&pStartId, &pEndId) == Result::True)
    {
        startId = uint64_t(pStartId);
        endId = uint64_t(pEndId);
        return true;
    }
    return false;
}

bool ax::NodeEditor::QueryNewLink(uint64_t& startId, uint64_t& endId, const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    startId = endId = 0;
    PinId pStartId, pEndId;
    auto result = context.QueryLink(&pStartId, &pEndId);
    if (result != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);

    if (result == Result::True)
    {
        startId = uint64_t(pStartId);
        endId = uint64_t(pEndId);
        return true;
    }
    return false;
}

bool ax::NodeEditor::QueryNewNode(uint64_t& pinId)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    pinId = 0;
    PinId pId;
    if (context.QueryNode(&pId) == Result::True)
    {
        pinId = uint64_t(pId);
        return true;
    }
    return false;
}

bool ax::NodeEditor::QueryNewNode(uint64_t& pinId, const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    pinId = 0;
    PinId pId;
    auto result = context.QueryNode(&pId);
    if (result != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);

    if (result == Result::True)
    {
        pinId = uint64_t(pId);
        return true;
    }
    return false;
}

bool ax::NodeEditor::AcceptNewItem()
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    return context.AcceptItem() == Result::True;
}

bool ax::NodeEditor::AcceptNewItem(const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    auto result = context.AcceptItem();
    if (result != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);

    return result == Result::True;
}

void ax::NodeEditor::RejectNewItem()
{
    auto& context = s_Editor->GetItemCreator();

    context.RejectItem();
}

void ax::NodeEditor::RejectNewItem(const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    if (context.RejectItem() != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);
}

void ax::NodeEditor::EndCreate()
{
    auto& context = s_Editor->GetItemCreator();

    context.End();
}

bool ax::NodeEditor::BeginDelete()
{
    auto& context = s_Editor->GetItemDeleter();

    return context.Begin();
}

bool ax::NodeEditor::QueryDeletedLink(uint64_t& linkId, uint64_t* startId, uint64_t* endId)
{
    auto& context = s_Editor->GetItemDeleter();

    linkId = 0;
    if (startId != nullptr) *startId = 0;
    if (endId != nullptr) *endId = 0;
    
    LinkId pLinkId;
    PinId pStartId, pEndId;
    if (context.QueryLink(&pLinkId, &pStartId, &pEndId))
    {
        linkId = uint64_t(pLinkId);
        if (startId != nullptr)
            *startId = uint64_t(pStartId);
        if (endId != nullptr)
            *endId = uint64_t(pEndId);
        return true;
    }
    return false;
}

bool ax::NodeEditor::QueryDeletedNode(uint64_t& nodeId)
{
    auto& context = s_Editor->GetItemDeleter();

    nodeId = 0;
    NodeId pNodeId;
    if (context.QueryNode(&pNodeId))
    {
        nodeId = uint64_t(pNodeId);
        return true;
    }
    return false;
}

bool ax::NodeEditor::AcceptDeletedItem(bool deleteDependencies)
{
    auto& context = s_Editor->GetItemDeleter();

    return context.AcceptItem(deleteDependencies);
}

void ax::NodeEditor::RejectDeletedItem()
{
    auto& context = s_Editor->GetItemDeleter();

    context.RejectItem();
}

void ax::NodeEditor::EndDelete()
{
    auto& context = s_Editor->GetItemDeleter();

    context.End();
}

void ax::NodeEditor::SetNodePosition(EdType& edNode, const ImVec2& position)
{
    if (edNode.has_object())
        s_Editor->SetNodePosition(edNode.get_node(), position);
    else
        edNode.init(s_Editor->SetNodePosition(edNode.id(), position));
}

void ax::NodeEditor::SetGroupSize(EdType& edNode, const ImVec2& size)
{
    if (edNode.has_object())
        s_Editor->SetGroupSize(edNode.get_node(), size);
    else
        edNode.init(s_Editor->SetGroupSize(edNode.id(), size));
}

ImVec2 ax::NodeEditor::GetNodePosition(const EdType& edNode)
{
    IM_ASSERT(edNode.has_object());
    return s_Editor->GetNodePosition(edNode.get_node());
}

ImVec2 ax::NodeEditor::GetNodeSize(const EdType& edNode)
{
    IM_ASSERT(edNode.has_object());
    return s_Editor->GetNodeSize(edNode.get_node());
}

void ax::NodeEditor::CenterNodeOnScreen(const EdType& edNode)
{
    IM_ASSERT(edNode.has_object());
    ax::NodeEditor::Detail::Node* pnode = edNode.get_node();
    pnode->CenterOnScreenInNextFrame();
}

void ax::NodeEditor::SetNodeZPosition(NodeId nodeId, float z)
{
    s_Editor->SetNodeZPosition(nodeId, z);
}

float ax::NodeEditor::GetNodeZPosition(NodeId nodeId)
{
    return s_Editor->GetNodeZPosition(nodeId);
}

//void ax::NodeEditor::RestoreNodeState(NodeId nodeId)
//{
    //if (auto node = s_Editor->FindNode(nodeId))
    //    s_Editor->MarkNodeToRestoreState(node);
//}

void ax::NodeEditor::Suspend()
{
    s_Editor->Suspend();
}

void ax::NodeEditor::Resume()
{
    s_Editor->Resume();
}

bool ax::NodeEditor::IsSuspended()
{
    return s_Editor->IsSuspended();
}

bool ax::NodeEditor::IsActive()
{
    return s_Editor->IsFocused();
}

bool ax::NodeEditor::HasSelectionChanged()
{
    return s_Editor->HasSelectionChanged();
}

int ax::NodeEditor::GetSelectedObjectCount()
{
    return (int)s_Editor->GetSelectedObjects().size();
}

int ax::NodeEditor::GetSelectedNodesCount()
{
    if (GetSelectedObjectCount() == 0)
        return 0;
    int count = 0;
    auto& sel = s_Editor->GetSelectedObjects();
    for (auto obj : sel)
    {
        if (obj->AsNode())
            count++;
    }
    return count;
}

int ax::NodeEditor::GetSelectedLinksCount()
{
    if (GetSelectedObjectCount() == 0)
        return 0;
    int count = 0;
    auto& sel = s_Editor->GetSelectedObjects();
    for (auto obj : sel)
    {
        if (obj->AsLink())
            count++;
    }
    return count;

}

std::set<uint64_t> ax::NodeEditor::GetSelectedNodes()
{
    std::set<uint64_t> ret;
    auto objects = s_Editor->GetSelectedObjects();
    for (auto obj : objects)
    {
        if (obj->AsNode() != nullptr)
        {
            ret.insert(uint64_t(obj->ID().AsPointer()));
        }
    }
    return ret;
}

std::set<uint64_t> ax::NodeEditor::GetSelectedLinks()
{
    std::set<uint64_t> ret;
    auto objects = s_Editor->GetSelectedObjects();
    for (auto obj : objects)
    {
        if (obj->AsLink() != nullptr)
        {
            ret.insert(uint64_t(obj->ID().AsPointer()));
        }
    }
    return ret;
}

bool ax::NodeEditor::IsNodeSelected(const EdType& edNode)
{
    IM_ASSERT(edNode.has_object());
    return s_Editor->IsSelected(edNode.get_object());
}

bool ax::NodeEditor::IsLinkSelected(const EdType& edLink)
{
    IM_ASSERT(edLink.has_object());
    return s_Editor->IsSelected(edLink.get_object());
}

void ax::NodeEditor::ClearSelection()
{
    s_Editor->ClearSelection();
}

void ax::NodeEditor::SelectNode(const EdType& edNode, bool append)
{
    IM_ASSERT(edNode.has_object());
    if (append)
        s_Editor->SelectObject(edNode.get_object());
    else
        s_Editor->SetSelectedObject(edNode.get_object());
}

void ax::NodeEditor::SelectLink(const EdType& edLink, bool append)
{
    IM_ASSERT(edLink.has_object());
    if (append)
        s_Editor->SelectObject(edLink.get_object());
    else
        s_Editor->SetSelectedObject(edLink.get_object());
}

void ax::NodeEditor::DeselectNode(const EdType& edNode)
{
    IM_ASSERT(edNode.has_object());
    s_Editor->DeselectObject(edNode.get_object());
}

void ax::NodeEditor::DeselectLink(const EdType& edLink)
{
    IM_ASSERT(edLink.has_object());
    s_Editor->DeselectObject(edLink.get_object());
}

void ax::NodeEditor::SetNodeDraggable(const EdType& edNode, bool canDrag)
{
    IM_ASSERT(edNode.has_object());
    ax::NodeEditor::Detail::Node* pnode = edNode.get_node();
    pnode->m_Draggable = canDrag;
}

bool ax::NodeEditor::DeleteNode(const EdType& edNode)
{
    if (edNode.has_object())
        return s_Editor->GetItemDeleter().Add(edNode.get_object());

    if (auto node = s_Editor->FindNode(edNode.id()))
        return s_Editor->GetItemDeleter().Add(node);

    return false;
}

bool ax::NodeEditor::DeleteLink(const EdType& edLink)
{
    if (edLink.has_object())
        return s_Editor->GetItemDeleter().Add(edLink.get_object());

    if (auto link = s_Editor->FindLink(edLink.id()))
        return s_Editor->GetItemDeleter().Add(link);

    return false;
}

bool ax::NodeEditor::HasAnyLinks(NodeId nodeId)
{
    return s_Editor->HasAnyLinks(nodeId);
}

bool ax::NodeEditor::HasAnyLinks(PinId pinId)
{
    return s_Editor->HasAnyLinks(pinId);
}

int ax::NodeEditor::BreakLinks(NodeId nodeId)
{
    return s_Editor->BreakLinks(nodeId);
}

int ax::NodeEditor::BreakLinks(PinId pinId)
{
    return s_Editor->BreakLinks(pinId);
}

void ax::NodeEditor::NavigateToContent(float duration)
{
    s_Editor->NavigateTo(s_Editor->GetContentBounds(), true, duration);
}

void ax::NodeEditor::NavigateToSelection(bool zoomIn, float duration)
{
    s_Editor->NavigateTo(s_Editor->GetSelectionBounds(), zoomIn, duration);
}

bool ax::NodeEditor::ShowNodeContextMenu(NodeId* nodeId)
{
    return s_Editor->GetContextMenu().ShowNodeContextMenu(nodeId);
}

bool ax::NodeEditor::ShowPinContextMenu(PinId* pinId)
{
    return s_Editor->GetContextMenu().ShowPinContextMenu(pinId);
}

bool ax::NodeEditor::ShowLinkContextMenu(LinkId* linkId)
{
    return s_Editor->GetContextMenu().ShowLinkContextMenu(linkId);
}

bool ax::NodeEditor::ShowBackgroundContextMenu()
{
    return s_Editor->GetContextMenu().ShowBackgroundContextMenu();
}

void ax::NodeEditor::EnableShortcuts(bool enable)
{
    s_Editor->EnableShortcuts(enable);
}

bool ax::NodeEditor::AreShortcutsEnabled()
{
    return s_Editor->AreShortcutsEnabled();
}

bool ax::NodeEditor::BeginShortcut()
{
    return s_Editor->GetShortcut().Begin();
}

bool ax::NodeEditor::AcceptCut()
{
    return s_Editor->GetShortcut().AcceptCut();
}

bool ax::NodeEditor::AcceptCopy()
{
    return s_Editor->GetShortcut().AcceptCopy();
}

bool ax::NodeEditor::AcceptPaste()
{
    return s_Editor->GetShortcut().AcceptPaste();
}

bool ax::NodeEditor::AcceptDuplicate()
{
    return s_Editor->GetShortcut().AcceptDuplicate();
}

bool ax::NodeEditor::AcceptCreateNode()
{
    return s_Editor->GetShortcut().AcceptCreateNode();
}

int ax::NodeEditor::GetActionContextSize()
{
    return static_cast<int>(s_Editor->GetShortcut().m_Context.size());
}

int ax::NodeEditor::GetActionContextNodes(NodeId* nodes, int size)
{
    return BuildIdList(s_Editor->GetSelectedObjects(), nodes, size, [](auto object)
    {
        return object->AsNode() != nullptr;
    });
}

int ax::NodeEditor::GetActionContextLinks(LinkId* links, int size)
{
    return BuildIdList(s_Editor->GetSelectedObjects(), links, size, [](auto object)
    {
        return object->AsLink() != nullptr;
    });
}

void ax::NodeEditor::EndShortcut()
{
    return s_Editor->GetShortcut().End();
}

float ax::NodeEditor::GetCurrentZoom()
{
    return s_Editor->GetView().InvScale;
}

void ax::NodeEditor::SetCurrentZoom(float zoom)
{
    s_Editor->EditView().InvScale = zoom;
}

const ImRect& ax::NodeEditor::GetCurrentView()
{
    return s_Editor->GetViewRect();
}

void ax::NodeEditor::SetCurrentView(const ImRect& rc)
{
    s_Editor->SetViewRect(rc);
}

uint64_t ax::NodeEditor::GetHoveredNode()
{
    return uint64_t(s_Editor->GetHoveredNode());
}

uint64_t ax::NodeEditor::GetHoveredPin()
{
    return uint64_t(s_Editor->GetHoveredPin());
}

uint64_t ax::NodeEditor::GetHoveredLink()
{
    return uint64_t(s_Editor->GetHoveredLink());
}

ax::NodeEditor::NodeId ax::NodeEditor::GetDoubleClickedNode()
{
    return s_Editor->GetDoubleClickedNode();
}

ax::NodeEditor::PinId ax::NodeEditor::GetDoubleClickedPin()
{
    return s_Editor->GetDoubleClickedPin();
}

ax::NodeEditor::LinkId ax::NodeEditor::GetDoubleClickedLink()
{
    return s_Editor->GetDoubleClickedLink();
}

bool ax::NodeEditor::IsBackgroundClicked()
{
    return s_Editor->IsBackgroundClicked();
}

bool ax::NodeEditor::IsBackgroundDoubleClicked()
{
    return s_Editor->IsBackgroundDoubleClicked();
}

ImGuiMouseButton ax::NodeEditor::GetBackgroundClickButtonIndex()
{
    return s_Editor->GetBackgroundClickButtonIndex();
}

ImGuiMouseButton ax::NodeEditor::GetBackgroundDoubleClickButtonIndex()
{
    return s_Editor->GetBackgroundDoubleClickButtonIndex();
}

bool ax::NodeEditor::GetLinkPins(const EdType& edLink, PinId* startPinId, PinId* endPinId)
{
    IM_ASSERT(edLink.has_object());
    const ax::NodeEditor::Detail::Link* link = edLink.get_link();

    if (startPinId)
        *startPinId = link->m_StartPin->m_ID;
    if (endPinId)
        *endPinId = link->m_EndPin->m_ID;

    return true;
}

bool ax::NodeEditor::PinHadAnyLinks(PinId pinId)
{
    return s_Editor->PinHadAnyLinks(pinId);
}

ImVec2 ax::NodeEditor::GetScreenSize()
{
    return s_Editor->GetRect().GetSize();
}

ImVec2 ax::NodeEditor::ScreenToCanvas(const ImVec2& pos)
{
    return s_Editor->ToCanvas(pos);
}

ImVec2 ax::NodeEditor::CanvasToScreen(const ImVec2& pos)
{
    return s_Editor->ToScreen(pos);
}

int ax::NodeEditor::GetNodeCount()
{
    return s_Editor->CountLiveNodes();
}

int ax::NodeEditor::GetOrderedNodeIds(NodeId* nodes, int size)
{
    return s_Editor->GetNodeIds(nodes, size);
}
