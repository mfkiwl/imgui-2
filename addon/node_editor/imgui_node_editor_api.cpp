//------------------------------------------------------------------------------
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
static thread_local ax::NodeEditor::Detail::EditorContext* s_Editor = nullptr;


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

const ax::NodeEditor::Config& ax::NodeEditor::GetConfig(EditorContext* ctx)
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

void ax::NodeEditor::Update()
{
    s_Editor->Update();
}

void ax::NodeEditor::BeginNode(NodeId id)
{
    s_Editor->GetNodeBuilder().Begin(id);
}

void ax::NodeEditor::BeginPin(PinId id, PinKind kind)
{
    s_Editor->GetNodeBuilder().BeginPin(id, kind);
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

bool ax::NodeEditor::Link(LinkId id, PinId startPinId, PinId endPinId, const ImVec4& color/* = ImVec4(1, 1, 1, 1)*/, float thickness/* = 1.0f*/)
{
    return s_Editor->DoLink(id, startPinId, endPinId, ImColor(color), thickness);
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

bool ax::NodeEditor::QueryNewLink(PinId* startId, PinId* endId)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    return context.QueryLink(startId, endId) == Result::True;
}

bool ax::NodeEditor::QueryNewLink(PinId* startId, PinId* endId, const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    auto result = context.QueryLink(startId, endId);
    if (result != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);

    return result == Result::True;
}

bool ax::NodeEditor::QueryNewNode(PinId* pinId)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    return context.QueryNode(pinId) == Result::True;
}

bool ax::NodeEditor::QueryNewNode(PinId* pinId, const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = s_Editor->GetItemCreator();

    auto result = context.QueryNode(pinId);
    if (result != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);

    return result == Result::True;
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

bool ax::NodeEditor::QueryDeletedLink(LinkId* linkId, PinId* startId, PinId* endId)
{
    auto& context = s_Editor->GetItemDeleter();

    return context.QueryLink(linkId, startId, endId);
}

bool ax::NodeEditor::QueryDeletedNode(NodeId* nodeId)
{
    auto& context = s_Editor->GetItemDeleter();

    return context.QueryNode(nodeId);
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

void ax::NodeEditor::SetNodePosition(NodeId nodeId, const ImVec2& position)
{
    s_Editor->SetNodePosition(nodeId, position);
}

void ax::NodeEditor::SetNodeSize(NodeId nodeId, const ImVec2& size)
{
    s_Editor->SetNodeSize(nodeId, size);
}

void ax::NodeEditor::SetGroupSize(NodeId nodeId, const ImVec2& size)
{
    s_Editor->SetGroupSize(nodeId, size);
}

void ax::NodeEditor::SetNodeChanged(NodeId nodeId)
{
    s_Editor->SetNodeChanged(nodeId);
}

ImVec2 ax::NodeEditor::GetGroupSize(NodeId nodeId)
{
    return s_Editor->GetGroupSize(nodeId);
}

ImVec2 ax::NodeEditor::GetNodePosition(NodeId nodeId)
{
    return s_Editor->GetNodePosition(nodeId);
}

ImVec2 ax::NodeEditor::GetNodeSize(NodeId nodeId)
{
    return s_Editor->GetNodeSize(nodeId);
}

void ax::NodeEditor::CenterNodeOnScreen(NodeId nodeId)
{
    if (auto node = s_Editor->FindNode(nodeId))
        node->CenterOnScreenInNextFrame();
}

void ax::NodeEditor::SetNodeZPosition(NodeId nodeId, float z)
{
    s_Editor->SetNodeZPosition(nodeId, z);
}

float ax::NodeEditor::GetNodeZPosition(NodeId nodeId)
{
    return s_Editor->GetNodeZPosition(nodeId);
}

void ax::NodeEditor::RestoreNodeState(NodeId nodeId)
{
    if (auto node = s_Editor->FindNode(nodeId))
        s_Editor->MarkNodeToRestoreState(node);
}

void ax::NodeEditor::SetPinChanged(PinId pinId)
{
    if (s_Editor) s_Editor->SetPinChanged(pinId);
}

void ax::NodeEditor::SetLinkChanged(LinkId linkId)
{
    if (s_Editor) s_Editor->SetLinkChanged(linkId);
}

bool ax::NodeEditor::HasStateChanged(StateType stateType, const imgui_json::value& state)
{
    switch (stateType)
    {
        case StateType::All:
            {
                Detail::EditorState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->HasStateChanged(value);
            }

        case StateType::Node:
            break;

        case StateType::Nodes:
            {
                Detail::NodesState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->HasStateChanged(value);
            }

        case StateType::Selection:
            {
                Detail::SelectionState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->HasStateChanged(value);
            }

        case StateType::View:
            {
                Detail::ViewState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->HasStateChanged(value);
            }
    }

    return false;
}

bool ax::NodeEditor::HasStateChanged(StateType stateType, NodeId nodeId, const imgui_json::value& state)
{
    switch (stateType)
    {
        case StateType::All:
            break;

        case StateType::Node:
            {
                Detail::NodeState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->ApplyState(nodeId, value);
            }

        case StateType::Nodes:
            break;

        case StateType::Selection:
            break;

        case StateType::View:
            break;
    }

    return false;
}


imgui_json::value ax::NodeEditor::GetState(StateType stateType)
{
    switch (stateType)
    {
        case StateType::All:
            {
                Detail::EditorState state;
                s_Editor->RecordState(state);
                return Detail::Serialization::ToJson(state);
            }

        case StateType::Node:
            break;

        case StateType::Nodes:
            {
                Detail::NodesState state;
                s_Editor->RecordState(state);
                return Detail::Serialization::ToJson(state);
            }

        case StateType::Selection:
            {
                Detail::SelectionState state;
                s_Editor->RecordState(state);
                return Detail::Serialization::ToJson(state);
            }

        case StateType::View:
            {
                Detail::ViewState state;
                s_Editor->RecordState(state);
                return Detail::Serialization::ToJson(state);
            }
    }

    return {};
}

imgui_json::value ax::NodeEditor::GetState(StateType stateType, NodeId nodeId)
{
    switch (stateType)
    {
        case StateType::All:
            break;

        case StateType::Node:
            {
                Detail::NodeState state;
                s_Editor->RecordState(nodeId, state);
                return Detail::Serialization::ToJson(state);
            }

        case StateType::Nodes:
            break;

        case StateType::Selection:
            break;

        case StateType::View:
            break;
    }

    return {};
}

bool ax::NodeEditor::ApplyState(StateType stateType, const imgui_json::value& state)
{
    switch (stateType)
    {
        case StateType::All:
            {
                Detail::EditorState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->ApplyState(value);
            }

        case StateType::Node:
            break;

        case StateType::Nodes:
            {
                Detail::NodesState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->ApplyState(value);
            }

        case StateType::Selection:
            {
                Detail::SelectionState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->ApplyState(value);
            }

        case StateType::View:
            {
                Detail::ViewState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->ApplyState(value);
            }
    }

    return false;
}

bool ax::NodeEditor::ApplyState(StateType stateType, NodeId nodeId, const imgui_json::value& state)
{
    switch (stateType)
    {
        case StateType::All:
            break;

        case StateType::Node:
            {
                Detail::NodeState value;
                return Detail::Serialization::Parse(state, value) && s_Editor->ApplyState(nodeId, value);
            }

        case StateType::Nodes:
            break;

        case StateType::Selection:
            break;

        case StateType::View:
            break;
    }

    return false;
}

bool ax::NodeEditor::HasStateChangedString(StateType stateType, const char* state)
{
    imgui_json::value value;
    return Detail::Serialization::Parse(state, value) && HasStateChanged(stateType, value);
}

bool ax::NodeEditor::HasStateChangedString(StateType stateType, NodeId nodeId, const char* state)
{
    imgui_json::value value;
    return Detail::Serialization::Parse(state, value) && HasStateChanged(stateType, nodeId, value);
}

const char* ax::NodeEditor::GetStateString(StateType stateType)
{
    s_Editor->m_CachedStateStringForPublicAPI = Detail::Serialization::ToString(GetState(stateType));
    return s_Editor->m_CachedStateStringForPublicAPI.c_str();
}

const char* ax::NodeEditor::GetStateString(StateType stateType, NodeId nodeId)
{
    s_Editor->m_CachedStateStringForPublicAPI = Detail::Serialization::ToString(GetState(stateType, nodeId));
    return s_Editor->m_CachedStateStringForPublicAPI.c_str();
}

bool ax::NodeEditor::ApplyStateString(StateType stateType, const char* state)
{
    imgui_json::value value;
    return Detail::Serialization::Parse(state, value) && ApplyState(stateType, value);
}

bool ax::NodeEditor::ApplyStateString(StateType stateType, NodeId nodeId, const char* state)
{
    imgui_json::value value;
    return Detail::Serialization::Parse(state, value) && ApplyState(stateType, nodeId, value);
}


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

int ax::NodeEditor::GetSelectedNodes(NodeId* nodes, int size)
{
    return BuildIdList(s_Editor->GetSelectedObjects(), nodes, size, [](auto object)
    {
        return object->AsNode() != nullptr;
    });
}

int ax::NodeEditor::GetGroupedNodes(std::vector<NodeId>& nodes, NodeId nodeId, ImVec2 expand)
{
    nodes.clear();
    auto current_node = s_Editor->GetNode(nodeId);
    if (current_node && current_node->m_Type == Detail::NodeType::Group)
    {
        std::vector<Detail::Node*> groupedNodes;
        current_node->GetGroupedNodes(groupedNodes, false, expand);
        for (auto node : groupedNodes)
        {
            nodes.push_back(node->m_ID);
        }
    }
    return nodes.size();
}

void ax::NodeEditor::SetNodeGroupID(NodeId nodeId, NodeId groupId)
{
    if (!s_Editor)
        return;
    auto current_node = s_Editor->GetNode(nodeId);
    if (current_node)
    {
        current_node->SetGroupID(groupId);
    }
}

int ax::NodeEditor::GetSelectedLinks(LinkId* links, int size)
{
    return BuildIdList(s_Editor->GetSelectedObjects(), links, size, [](auto object)
    {
        return object->AsLink() != nullptr;
    });
}

bool ax::NodeEditor::IsNodeSelected(NodeId nodeId)
{
    if (auto node = s_Editor->FindNode(nodeId))
        return s_Editor->IsSelected(node);
    else
        return false;
}

bool ax::NodeEditor::IsLinkSelected(LinkId linkId)
{
    if (auto link = s_Editor->FindLink(linkId))
        return s_Editor->IsSelected(link);
    else
        return false;
}

void ax::NodeEditor::ClearSelection()
{
    s_Editor->ClearSelection();
}

void ax::NodeEditor::SelectNode(NodeId nodeId, bool append)
{
    if (auto node = s_Editor->FindNode(nodeId))
    {
        if (append)
            s_Editor->SelectObject(node);
        else
            s_Editor->SetSelectedObject(node);
    }
}

void ax::NodeEditor::SelectLink(LinkId linkId, bool append)
{
    if (auto link = s_Editor->FindLink(linkId))
    {
        if (append)
            s_Editor->SelectObject(link);
        else
            s_Editor->SetSelectedObject(link);
    }
}

void ax::NodeEditor::DeselectNode(NodeId nodeId)
{
    if (auto node = s_Editor->FindNode(nodeId))
        s_Editor->DeselectObject(node);
}

void ax::NodeEditor::DeselectLink(LinkId linkId)
{
    if (auto link = s_Editor->FindLink(linkId))
        s_Editor->DeselectObject(link);
}

bool ax::NodeEditor::DeleteNode(NodeId nodeId)
{
    if (auto node = s_Editor->FindNode(nodeId))
        return s_Editor->GetItemDeleter().Add(node);
    else
        return false;
}

bool ax::NodeEditor::DeleteLink(LinkId linkId)
{
    if (auto link = s_Editor->FindLink(linkId))
        return s_Editor->GetItemDeleter().Add(link);
    else
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

void ax::NodeEditor::NavigateToOrigin(float duration)
{
    s_Editor->NavigateTo(s_Editor->GetRect(), true, duration);
}

void ax::NodeEditor::SetTheme(std::string theme)
{
    s_Editor->SetTheme(theme);
}

std::string ax::NodeEditor::GetTheme()
{
    return s_Editor->GetTheme();
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

void ax::NodeEditor::EnableDragOverBorder(bool enable)
{
    s_Editor->EnableDragOverBorder(enable);
}

void ax::NodeEditor::TriggerShowMeters()
{
    s_Editor->TriggerShowMeters();
}

bool ax::NodeEditor::AreShortcutsEnabled()
{
    return s_Editor->AreShortcutsEnabled();
}

bool ax::NodeEditor::AreDragOverBorderEnabled()
{
    return s_Editor->AreDragOverBorderEnabled();
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

ImVec2 ax::NodeEditor::GetCurrentOrigin()
{
    return s_Editor->GetView().Origin;
}

ax::NodeEditor::NodeId ax::NodeEditor::GetHoveredNode()
{
    return s_Editor->GetHoveredNode();
}

ax::NodeEditor::PinId ax::NodeEditor::GetHoveredPin()
{
    return s_Editor->GetHoveredPin();
}

ax::NodeEditor::LinkId ax::NodeEditor::GetHoveredLink()
{
    return s_Editor->GetHoveredLink();
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

bool ax::NodeEditor::GetLinkPins(LinkId linkId, PinId* startPinId, PinId* endPinId)
{
    auto link = s_Editor->FindLink(linkId);
    if (!link)
        return false;

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

ImVec2 ax::NodeEditor::GetViewSize()
{
    return s_Editor->GetViewRect().GetSize();
}

ImRect ax::NodeEditor::GetViewRect()
{
    return s_Editor->GetViewRect();
}

int ax::NodeEditor::GetNodeCount()
{
    return s_Editor->CountLiveNodes();
}

int ax::NodeEditor::GetOrderedNodeIds(NodeId* nodes, int size)
{
    return s_Editor->GetNodeIds(nodes, size);
}

void ax::NodeEditor::DrawLastLine(bool light)
{
    return s_Editor->DrawLastLine(light);
}

ImVector<ax::NodeEditor::LinkId> ax::NodeEditor::FindLinksForNode(NodeId nodeId)
{
    ImVector<LinkId> result;

    std::vector<ax::NodeEditor::Detail::Link*> links;
    s_Editor->FindLinksForNode(nodeId, links, false);

    result.reserve(static_cast<int>(links.size()));

    for (auto link : links)
        result.push_back(link->m_ID);

    return result;
}