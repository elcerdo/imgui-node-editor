#pragma once
#include "Types.h"
#include "ImGuiInterop.h"
#include "EditorApi.h"
#define PICOJSON_USE_LOCALE 0
#include "picojson.h"
#include <vector>

namespace ax {
namespace Editor {
namespace Detail {

using namespace ImGuiInterop;
using std::vector;
using std::string;


//------------------------------------------------------------------------------
void Log(const char* fmt, ...);


//------------------------------------------------------------------------------
enum class ObjectType
{
    Node, Pin
};

enum class NodeStage
{
    Invalid,
    Begin,
    Header,
    Content,
    Input,
    Output,
    End
};

using ax::Editor::PinKind;
using ax::Editor::Style;
using ax::Editor::StyleColor;

struct Node;
struct Pin;
struct Link;

struct Object
{
    int     ID;
    bool    IsLive;

    Object(int id): ID(id), IsLive(true) {}
    virtual ~Object() = default;

    virtual Node* AsNode() { return nullptr; }
    virtual Pin*  AsPin()  { return nullptr; }
    virtual Link* AsLink() { return nullptr; }
};

struct Pin final: Object
{
    PinKind Kind;
    Node*   Node;
    rect    Bounds;
    pointf  DragPoint;
    Pin*    PreviousPin;

    Pin(int id, PinKind kind):
        Object(id), Kind(kind), Node(nullptr), Bounds(), PreviousPin(nullptr)
    {
    }

    virtual Pin* AsPin() override final { return this; }
};

struct Node final: Object
{
    rect    Bounds;
    int     Channel;
    Pin*    LastPin;
    point   DragStart;

    Node(int id):
        Object(id),
        Bounds(point(0, 0), size()),
        Channel(0),
        LastPin(nullptr),
        DragStart()
    {
    }

    virtual Node* AsNode() override final { return this; }
};

struct Link final: Object
{
    Pin*  StartPin;
    Pin*  EndPin;
    ImU32 Color;
    float Thickness;

    Link(int id):
        Object(id), StartPin(nullptr), EndPin(nullptr), Color(IM_COL32_WHITE), Thickness(1.0f)
    {
    }

    virtual Link* AsLink() override final { return this; }
};

struct NodeSettings
{
    int    ID;
    ImVec2 Location;
    bool   WasUsed;

    NodeSettings(int id): ID(id), WasUsed(false) {}
};

struct Settings
{
    bool    Dirty;

    vector<NodeSettings> Nodes;
    ImVec2               ViewScroll;
    float                ViewZoom;

    Settings(): Dirty(false), ViewScroll(0, 0), ViewZoom(1.0f) {}
};

struct Control
{
    Object* HotObject;
    Object* ActiveObject;
    Object* ClickedObject;
    Node*   HotNode;
    Node*   ActiveNode;
    Node*   ClickedNode;
    Pin*    HotPin;
    Pin*    ActivePin;
    Pin*    ClickedPin;
    Link*   HotLink;
    Link*   ActiveLink;
    Link*   ClickedLink;
    bool    BackgroundHot;
    bool    BackgroundActive;
    bool    BackgroundClicked;

    Control(Object* hotObject, Object* activeObject, Object* clickedObject,
        bool backgroundHot, bool backgroundActive, bool backgroundClicked):
        HotObject(hotObject),
        ActiveObject(activeObject),
        ClickedObject(clickedObject),
        HotNode(nullptr),
        ActiveNode(nullptr),
        ClickedNode(nullptr),
        HotPin(nullptr),
        ActivePin(nullptr),
        ClickedPin(nullptr),
        HotLink(nullptr),
        ActiveLink(nullptr),
        ClickedLink(nullptr),
        BackgroundHot(backgroundHot),
        BackgroundActive(backgroundActive),
        BackgroundClicked(backgroundClicked)
    {
        if (hotObject)
        {
            HotNode = hotObject->AsNode();
            HotPin  = hotObject->AsPin();
            HotLink = hotObject->AsLink();

            if (HotPin)
                HotNode = HotPin->Node;
        }

        if (activeObject)
        {
            ActiveNode = activeObject->AsNode();
            ActivePin  = activeObject->AsPin();
            ActiveLink = activeObject->AsLink();
        }

        if (clickedObject)
        {
            ClickedNode = clickedObject->AsNode();
            ClickedPin  = clickedObject->AsPin();
            ClickedLink = clickedObject->AsLink();
        }
    }
};

// Spaces:
//   Canvas - where objects are
//   Client - where objects are drawn
//   Screen - global screen space
struct Canvas
{
    ImVec2 WindowScreenPos;
    ImVec2 WindowScreenSize;
    ImVec2 ClientOrigin;
    ImVec2 ClientSize;
    ImVec2 Zoom;
    ImVec2 InvZoom;

    Canvas();
    Canvas(ImVec2 position, ImVec2 size, ImVec2 scale, ImVec2 origin);

    ImVec2 FromScreen(ImVec2 point);
    ImVec2 ToScreen(ImVec2 point);
    ImVec2 FromClient(ImVec2 point);
    ImVec2 ToClient(ImVec2 point);
};

struct Context;
struct ScrollAction;
struct DragAction;
struct SelectAction;
struct CreateItemAction;
struct DeleteItemsAction;

struct EditorAction
{
    EditorAction(Context* editor): Editor(editor) {}
    virtual ~EditorAction() {}

    virtual const char* GetName() const = 0;

    virtual bool Accept(const Control& control) = 0;
    virtual bool Process(const Control& control) = 0;

    virtual void ShowMetrics() {}

    virtual ScrollAction*      AsScroll()      { return nullptr; }
    virtual DragAction*        AsDrag()        { return nullptr; }
    virtual SelectAction*      AsSelect()      { return nullptr; }
    virtual CreateItemAction*  AsCreateItem()  { return nullptr; }
    virtual DeleteItemsAction* AsDeleteItems() { return nullptr; }

    Context* Editor;
};

struct ScrollAction final: EditorAction
{
    bool   IsActive;
    float  Zoom;
    ImVec2 Scroll;

    ScrollAction(Context* editor);

    virtual const char* GetName() const override final { return "Scroll"; }

    virtual bool Accept(const Control& control) override final;
    virtual bool Process(const Control& control) override final;

    virtual void ShowMetrics() override final;

    virtual ScrollAction* AsScroll() override final { return this; }

    void SetWindow(ImVec2 position, ImVec2 size);

    Canvas GetCanvas();

private:
    ImVec2 WindowScreenPos;
    ImVec2 WindowScreenSize;
    ImVec2 ScrollStart;

    float MatchZoom(int steps, float fallbackZoom);
    int MatchZoomIndex();

    static const float s_ZoomLevels[];
    static const int   s_ZoomLevelCount;
};

struct DragAction final: EditorAction
{
    bool  IsActive;
    Node* DraggedNode;

    DragAction(Context* editor);

    virtual const char* GetName() const override final { return "Drag"; }

    virtual bool Accept(const Control& control) override final;
    virtual bool Process(const Control& control) override final;

    virtual void ShowMetrics() override final;

    virtual DragAction* AsDrag() override final { return this; }
};

struct SelectAction final: EditorAction
{
    bool            IsActive;

    bool            SelectLinkMode;
    ImVec2          StartPoint;
    ImVec2          EndPoint;
    vector<Object*> CandidateObjects;
    vector<Object*> SelectedObjectsAtStart;

    SelectAction(Context* editor);

    virtual const char* GetName() const override final { return "Select"; }

    virtual bool Accept(const Control& control) override final;
    virtual bool Process(const Control& control) override final;

    virtual void ShowMetrics() override final;

    virtual SelectAction* AsSelect() override final { return this; }
};

struct CreateItemAction final : EditorAction
{
    enum Stage
    {
        None,
        Possible,
        Create
    };

    enum Action
    {
        Unknown,
        UserReject,
        UserAccept
    };

    enum Type
    {
        NoItem,
        Node,
        Link
    };

    enum Result
    {
        True,
        False,
        Indeterminate
    };

    bool      InActive;
    Stage     NextStage;

    Stage     CurrentStage;
    Type      ItemType;
    Action    UserAction;
    ImU32     LinkColor;
    float     LinkThickness;
    Pin*      LinkStart;
    Pin*      LinkEnd;

    bool      IsActive;
    Pin*      DraggedPin;


    CreateItemAction(Context* editor);

    virtual const char* GetName() const override final { return "Create Item"; }

    virtual bool Accept(const Control& control) override final;
    virtual bool Process(const Control& control) override final;

    virtual void ShowMetrics() override final;

    virtual CreateItemAction* AsCreateItem() override final { return this; }

    void SetStyle(ImU32 color, float thickness);

    bool Begin();
    void End();

    Result RejectItem();
    Result AcceptItem();

    Result QueryLink(int* startId, int* endId);
    Result QueryNode(int* pinId);

private:
    void DragStart(Pin* startPin);
    void DragEnd();
    void DropPin(Pin* endPin);
    void DropNode();
    void DropNothing();

    void SetUserContext();
};

struct DeleteItemsAction final: EditorAction
{
    bool    IsActive;
    bool    InInteraction;

    DeleteItemsAction(Context* editor);

    virtual const char* GetName() const override final { return "Delete Items"; }

    virtual bool Accept(const Control& control) override final;
    virtual bool Process(const Control& control) override final;

    virtual void ShowMetrics() override final;

    virtual DeleteItemsAction* AsDeleteItems() override final { return this; }

    bool Begin();
    void End();

    bool QueryLink(int* linkId);
    bool QueryNode(int* nodeId);

    bool AcceptItem();
    void RejectItem();

private:
    enum IteratorType { Unknown, Link, Node };
    enum UserAction { Undetermined, Accepted, Rejected };

    bool QueryItem(int* itemId, IteratorType itemType);
    void RemoveItem();

    IteratorType    CurrentItemType;
    UserAction      UserAction;
    vector<Object*> CandidateObjects;
    int             CandidateItemIndex;
};

struct NodeBuilder
{
    Context* const Editor;

    Node* CurrentNode;
    Pin*  CurrentPin;

    rect NodeRect;

    NodeBuilder(Context* editor);

    void Begin(int nodeId);
    void End();

    void BeginPin(int pinId, PinKind kind, const ImVec2& pivot);
    void EndPin();

private:
    void DrawBackground() const;
};

struct Context
{
    Context(const Config* config = nullptr);
    ~Context();

    Style& GetStyle() { return Style; }
    const char* GetStyleColorName(StyleColor colorIndex) const;

    void Begin(const char* id, const ImVec2& size = ImVec2(0, 0));
    void End();

    void BeginNode(int id);
    void EndNode();

    void BeginHeader(ImU32 color);
    void EndHeader();

    void BeginInput(int id);
    void EndInput();

    void BeginOutput(int id);
    void EndOutput();

    bool DoLink(int id, int startPinId, int endPinId, ImU32 color, float thickness);

    NodeBuilder& GetNodeBuilder() { return NodeBuilder; }

    EditorAction* GetCurrentAction() { return CurrentAction; }

    CreateItemAction& GetItemCreator() { return CreateItemAction; }
    DeleteItemsAction& GetItemDeleter() { return DeleteItemsAction; }

    void SetNodePosition(int nodeId, const ImVec2& screenPosition);
    ImVec2 GetNodePosition(int nodeId);

    void ClearSelection();
    void SelectObject(Object* object);
    void DeselectObject(Object* object);
    void SetSelectedObject(Object* object);
    void ToggleObjectSelection(Object* object);
    bool IsSelected(Object* object);
    const vector<Object*>& GetSelectedObjects();
    bool IsAnyNodeSelected();
    bool IsAnyLinkSelected();
    bool HasSelectionChanged();

    void FindNodesInRect(ax::rect r, vector<Node*>& result);
    void FindLinksInRect(ax::rect r, vector<Link*>& result);

    void FindLinksForNode(int nodeId, vector<Link*>& result, bool add = false);

    ImVec2 ToCanvas(ImVec2 point) { return Canvas.FromScreen(point); }
    ImVec2 ToScreen(ImVec2 point) { return Canvas.ToScreen(point); }

    void NotifyLinkDeleted(Link* link);

    void Suspend();
    void Resume();

    void MarkSettingsDirty();

    Pin*    CreatePin(int id, PinKind kind);
    Node*   CreateNode(int id);
    Link*   CreateLink(int id);
    void    DestroyObject(Node* node);
    Object* FindObject(int id);

    Node* FindNode(int id);
    Pin*  FindPin(int id);
    Link* FindLink(int id);

    Node* GetNode(int id);

    Pin* GetPin(int id, PinKind kind);
    void BeginPin(int id, PinKind kind);
    void EndPin();

    Link* GetLink(int id);

    Link* FindLinkAt(const point& p);

    ImU32 GetColor(StyleColor colorIndex) const;
    ImU32 GetColor(StyleColor colorIndex, float alpha) const;

private:
    void SetCurrentNode(Node* node);
    void SetCurrentPin(Pin* pin);

    bool SetNodeStage(NodeStage stage);

    NodeSettings* FindNodeSettings(int id);
    NodeSettings* AddNodeSettings(int id);
    void          LoadSettings();
    void          SaveSettings();

    Control ComputeControl();

    void ShowMetrics(const Control& control);

    void CaptureMouse();
    void ReleaseMouse();

    Style               Style;

    vector<Node*>       Nodes;
    vector<Pin*>        Pins;
    vector<Link*>       Links;

    Object*             SelectedObject;
    vector<Object*>     SelectedObjects;
    bool                SelectionChanged;

    Link*               LastActiveLink;

    Pin*                CurrentPin;
    Node*               CurrentNode;

    ImVec2              MousePosBackup;
    ImVec2              MouseClickPosBackup[5];

    Canvas              Canvas;

    bool                IsSuspended;

    // Node building
    NodeStage           NodeBuildStage;
    ImU32               HeaderColor;
    rect                NodeRect;
    rect                HeaderRect;
    rect                ContentRect;

    NodeBuilder         NodeBuilder;

    EditorAction*       CurrentAction;
    ScrollAction        ScrollAction;
    DragAction          DragAction;
    SelectAction        SelectAction;
    CreateItemAction    CreateItemAction;
    DeleteItemsAction   DeleteItemsAction;

    bool                IsInitialized;
    ImTextureID         HeaderTextureID;
    Settings            Settings;

    Config              Config;
};

} // namespace Detail
} // namespace Editor
} // namespace ax