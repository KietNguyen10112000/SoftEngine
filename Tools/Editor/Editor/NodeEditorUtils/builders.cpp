//------------------------------------------------------------------------------
// LICENSE
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
//
// CREDITS
//   Written by Michal Cichon
//------------------------------------------------------------------------------
# define IMGUI_DEFINE_MATH_OPERATORS
# include "builders.h"
# include <imgui_internal.h>

#include "imgui/imgui.h"

#include "imgui-node-editor/imgui_node_editor_internal.h"


//------------------------------------------------------------------------------
namespace ed   = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

util::BlueprintNodeBuilder::BlueprintNodeBuilder(ImTextureID texture, int textureWidth, int textureHeight):
    HeaderTextureId(texture),
    HeaderTextureWidth(textureWidth),
    HeaderTextureHeight(textureHeight),
    CurrentNodeId(0),
    CurrentStage(Stage::Invalid),
    HasHeader(false)
{
}

void util::BlueprintNodeBuilder::Begin(ed::NodeId id)
{
    HasHeader  = false;
    HeaderMin = HeaderMax = ImVec2();

    ed::PushStyleVar(StyleVar_NodePadding, ImVec4(8, 4, 8, 8));

    ed::BeginNode(id);

    ImGui::PushID(id.AsPointer());
    CurrentNodeId = id;

    SetStage(Stage::Begin);
}

void util::BlueprintNodeBuilder::End()
{
    SetStage(Stage::End);

    ed::EndNode();

    HeaderMin = ImGui::GetItemRectMin();

    auto nodeMax = ImGui::GetItemRectMax();
    HeaderMax.x = nodeMax.x;

    if (ImGui::IsItemVisible())
    {
        auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

        auto drawList = ed::GetNodeBackgroundDrawList(CurrentNodeId);

        const auto halfBorderWidth = ed::GetStyle().NodeBorderWidth * 0.5f;

        auto headerColor = IM_COL32(0, 0, 0, alpha) | (HeaderColor & IM_COL32(255, 255, 255, 0));
        if ((HeaderMax.x > HeaderMin.x) && (HeaderMax.y > HeaderMin.y) && HeaderTextureId)
        {
            const auto uv = ImVec2(
                (HeaderMax.x - HeaderMin.x) / (float)(4.0f * HeaderTextureWidth),
                (HeaderMax.y - HeaderMin.y) / (float)(4.0f * HeaderTextureHeight));

            drawList->AddImageRounded(HeaderTextureId,
                HeaderMin - ImVec2(0 - halfBorderWidth, 0 - halfBorderWidth),
                HeaderMax + ImVec2(0 - halfBorderWidth, 0),
                ImVec2(0.0f, 0.0f), uv,
#if IMGUI_VERSION_NUM > 18101
                headerColor, GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);
#else
                headerColor, GetStyle().NodeRounding, 1 | 2);
#endif

            if (ContentMin.y > HeaderMax.y)
            {
                drawList->AddLine(
                    ImVec2(HeaderMin.x - (0 - halfBorderWidth), HeaderMax.y - 0.5f),
                    ImVec2(HeaderMax.x + (0 - halfBorderWidth), HeaderMax.y - 0.5f),
                    ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
            }
        }

        for (auto& drawCall : bgDrawCalls)
        {
            drawCall.payloadCount = 0;

            switch (drawCall.type)
            {
            case SEPARATOR:
                SeparatorImpl(drawList, &drawCall);
                break;
            default:
                break;
            }
        }
        bgDrawCalls.clear();
    }

    CurrentNodeId = 0;

    ImGui::PopID();

    ed::PopStyleVar();

    SetStage(Stage::Invalid);
}

void util::BlueprintNodeBuilder::Header(const ImVec4& color)
{
    HeaderColor = ImColor(color);
    SetStage(Stage::Header);
}

void util::BlueprintNodeBuilder::EndHeader()
{
    SetStage(Stage::Content);
}

void util::BlueprintNodeBuilder::Input(ed::PinId id)
{
    if (CurrentStage == Stage::Begin)
        SetStage(Stage::Content);

    const auto applyPadding = (CurrentStage == Stage::Input);

    SetStage(Stage::Input);

    //if (applyPadding)
    //    ImGui::Spring(0);

    Pin(id, PinKind::Input);

    //ImGui::BeginHorizontal(id.AsPointer());
}

void util::BlueprintNodeBuilder::EndInput()
{
    //ImGui::EndHorizontal();

    EndPin();
}

void util::BlueprintNodeBuilder::Middle()
{
    if (CurrentStage == Stage::Begin)
        SetStage(Stage::Content);

    SetStage(Stage::Middle);
}

void util::BlueprintNodeBuilder::Output(ed::PinId id)
{
    if (CurrentStage == Stage::Begin)
        SetStage(Stage::Content);

    const auto applyPadding = (CurrentStage == Stage::Output);

    SetStage(Stage::Output);

    //if (applyPadding)
    //    ImGui::Spring(0);

    Pin(id, PinKind::Output);

    //ImGui::BeginHorizontal(id.AsPointer());
}

void util::BlueprintNodeBuilder::EndOutput()
{
    //ImGui::EndHorizontal();

    EndPin();
}

void ax::NodeEditor::Utilities::BlueprintNodeBuilder::Separator()
{
    auto& drawCall = bgDrawCalls.emplace_back();
    drawCall.type = BG_DRAW_TYPE::SEPARATOR;

    ImGui::Dummy({ 3,3 });

    auto pos = ImGui::GetItemRectMax();
    drawCall.NextVar<ImVec2>() = pos;

    ImGui::Dummy({ 3,3 });
}

void ax::NodeEditor::Utilities::BlueprintNodeBuilder::SeparatorImpl(ImDrawList* drawList, BgDrawCall* call)
{
    auto& pos = call->NextVar<ImVec2>();

    auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);
    //auto drawList = ed::GetNodeBackgroundDrawList(CurrentNodeId); //ed::GetCurrentEditor()->GetDrawList();

    auto nodeMin = ImGui::GetItemRectMin();
    auto nodeMax = ImGui::GetItemRectMax();

    //const auto halfBorderWidth = ed::GetStyle().NodeBorderWidth * 0.5f;

    auto headerColor = IM_COL32(0, 0, 0, alpha) | (HeaderColor & IM_COL32(255, 255, 255, 0));

    drawList->AddLine(
        ImVec2(nodeMin.x + 3, pos.y),
        ImVec2(nodeMin.x + (nodeMax.x - nodeMin.x) -3, pos.y),
        ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
}

bool util::BlueprintNodeBuilder::SetStage(Stage stage)
{
    if (stage == CurrentStage)
        return false;

    auto oldStage = CurrentStage;
    CurrentStage = stage;

    ImVec2 cursor;
    switch (oldStage)
    {
        case Stage::Begin:
            break;

        case Stage::Header:
            //ImGui::EndHorizontal();
            HeaderMin = ImGui::GetItemRectMin();
            HeaderMax = ImGui::GetItemRectMax();

            // spacing between header and content
            //ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.y * 2.0f);

            break;

        case Stage::Content:
            break;

        case Stage::Input:
            ed::PopStyleVar(2);

            //ImGui::Spring(1, 0);
            //ImGui::EndVertical();

            // #debug
            // ImGui::GetWindowDrawList()->AddRect(
            //     ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 0, 0, 255));

            break;

        case Stage::Middle:
            //ImGui::EndVertical();

            // #debug
            // ImGui::GetWindowDrawList()->AddRect(
            //     ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 0, 0, 255));

            break;

        case Stage::Output:
            ed::PopStyleVar(2);

            //ImGui::Spring(1, 0);
            //ImGui::EndVertical();

            // #debug
            // ImGui::GetWindowDrawList()->AddRect(
            //     ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 0, 0, 255));

            break;

        case Stage::End:
            break;

        case Stage::Invalid:
            break;
    }

    switch (stage)
    {
        case Stage::Begin:
            //ImGui::BeginVertical("node");
            break;

        case Stage::Header:
            HasHeader = true;

            //ImGui::BeginHorizontal("header");
            break;

        case Stage::Content:
            //if (oldStage == Stage::Begin)
                //ImGui::Spring(0);

            //ImGui::BeginHorizontal("content");
            //ImGui::Spring(0, 0);
            break;

        case Stage::Input:
            //ImGui::BeginVertical("inputs", ImVec2(0, 0), 0.0f);

            ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(0, 0.5f));
            ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));

            //if (!HasHeader)
            //    ImGui::Spring(1, 0);
            break;

        case Stage::Middle:
            //ImGui::Spring(1);
            //ImGui::BeginVertical("middle", ImVec2(0, 0), 1.0f);
            break;

        case Stage::Output:
            /*if (oldStage == Stage::Middle || oldStage == Stage::Input)
                ImGui::Spring(1);
            else
                ImGui::Spring(1, 0);
            ImGui::BeginVertical("outputs", ImVec2(0, 0), 1.0f);*/

            ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(1.0f, 0.5f));
            ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));

            //if (!HasHeader)
            //    ImGui::Spring(1, 0);
            break;

        case Stage::End:
            //if (oldStage == Stage::Input)
            //    ImGui::Spring(1, 0);
            //if (oldStage != Stage::Begin)
            //    ImGui::EndHorizontal();
            ContentMin = ImGui::GetItemRectMin();
            ContentMax = ImGui::GetItemRectMax();

            //ImGui::Spring(0);
            //ImGui::EndVertical();
            NodeMin = ImGui::GetItemRectMin();
            NodeMax = ImGui::GetItemRectMax();
            break;

        case Stage::Invalid:
            break;
    }

    return true;
}

void util::BlueprintNodeBuilder::Pin(ed::PinId id, ed::PinKind kind)
{
    ed::BeginPin(id, kind);
}

void util::BlueprintNodeBuilder::EndPin()
{
    ed::EndPin();

    // #debug
    // ImGui::GetWindowDrawList()->AddRectFilled(
    //     ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 0, 0, 64));
}


// =============================================================================================

namespace ax
{
    namespace NodeEditor 
    {
        static float CalcMaxPopupHeightFromItemCount(int items_count)
        {
            ImGuiContext& g = *GImGui;
            if (items_count <= 0)
                return FLT_MAX;
            return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
        }

        bool BeginComboPopup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags)
        {
            using namespace ::ImGui;

            ImGuiContext& g = *GImGui;
            if (!IsPopupOpen(popup_id, ImGuiPopupFlags_None))
            {
                g.NextWindowData.ClearFlags();
                return false;
            }

            // Set popup size
            float w = bb.GetWidth();
            if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
            {
                g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
            }
            else
            {
                if ((flags & ImGuiComboFlags_HeightMask_) == 0)
                    flags |= ImGuiComboFlags_HeightRegular;
                IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_)); // Only one
                int popup_max_height_in_items = -1;
                if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
                else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
                else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
                ImVec2 constraint_min(0.0f, 0.0f), constraint_max(FLT_MAX, FLT_MAX);
                if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.x <= 0.0f) // Don't apply constraints if user specified a size
                    constraint_min.x = w;
                if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.y <= 0.0f)
                    constraint_max.y = CalcMaxPopupHeightFromItemCount(popup_max_height_in_items);
                SetNextWindowSizeConstraints(constraint_min, constraint_max);
            }

            // This is essentially a specialized version of BeginPopupEx()
            char name[16];
            ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

            // Set position given a custom constraint (peak into expected window size so we can position it)
            // FIXME: This might be easier to express with an hypothetical SetNextWindowPosConstraints() function?
            // FIXME: This might be moved to Begin() or at least around the same spot where Tooltips and other Popups are calling FindBestWindowPosForPopupEx()?
            if (ImGuiWindow* popup_window = FindWindowByName(name))
                if (popup_window->WasActive)
                {
                    // Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
                    ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
                    popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
                    ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
                    ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
                    SetNextWindowPos(ax::NodeEditor::CanvasToScreen(pos));
                }

            // We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

            ax::NodeEditor::Suspend();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(g.Style.FramePadding.x, g.Style.WindowPadding.y)); // Horizontally align ourselves with the framed text
            bool ret = ImGui::Begin(name, NULL, window_flags);
            ImGui::PopStyleVar();
            if (!ret)
            {
                EndPopup();
                IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
                return false;
            }
            return true;
        }

    }
}

bool ax::NodeEditor::BeginNodeCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
    using namespace ImGui;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

    const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : CalcItemWidth();
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(bb.Min, bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &bb))
        return false;

    // Open on click
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !popup_open)
    {
        OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    // Render shape
    const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
    RenderNavHighlight(bb, id);
    if (!(flags & ImGuiComboFlags_NoPreview))
        window->DrawList->AddRectFilled(bb.Min, ImVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
    if (!(flags & ImGuiComboFlags_NoArrowButton))
    {
        ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ImU32 text_col = GetColorU32(ImGuiCol_Text);
        window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
        if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
            RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);
    }
    RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    // Custom preview
    if (flags & ImGuiComboFlags_CustomPreview)
    {
        g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
        IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
        preview_value = NULL;
    }

    // Render preview and label
    if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
    {
        if (g.LogEnabled)
            LogSetNextTextDecoration("{", "}");
        RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
    }
    if (label_size.x > 0)
        RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), label);

    if (!popup_open)
        return false;

    g.NextWindowData.Flags = backup_next_window_data_flags;
    return ed::BeginComboPopup(popup_id, bb, flags);
}

void ax::NodeEditor::EndNodeCombo()
{
    ImGui::EndPopup();

    ax::NodeEditor::Resume();
}