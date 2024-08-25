#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace ImGui
{

inline bool ToggleButton(const char* str_id, bool* v)
{
    auto preV = *v;

	ImVec4* colors = ImGui::GetStyle().Colors;
	ImVec2 p = ImGui::GetCursorScreenPos();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	float height = ImGui::GetFrameHeight() - 5.0f;
	float width = height * 1.55f;
	float radius = height * 0.50f;
    p.y += 0.5f;

	ImGui::InvisibleButton(str_id, ImVec2(width, height));
	if (ImGui::IsItemClicked()) *v = !*v;
	ImGuiContext& gg = *GImGui;
	float ANIM_SPEED = 0.085f;
	if (gg.LastActiveId == gg.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
		float t_anim = ImSaturate(gg.LastActiveIdTimer / ANIM_SPEED);
	if (ImGui::IsItemHovered())
		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * 0.5f);
	else
		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), height * 0.50f);
	draw_list->AddCircleFilled(ImVec2(p.x + radius + (*v ? 1 : 0) * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));

    return preV != *v;
}

inline bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = size_arg;
    size.x -= style.FramePadding.x * 2;

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    // Render
    const float circleStart = size.x * 0.7f;
    const float circleEnd = size.x;
    const float circleWidth = circleEnd - circleStart;

    window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
    window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

    const float t = g.Time;
    const float r = size.y / 2;
    const float speed = 1.5f;

    const float a = speed * 0;
    const float b = speed * 0.333f;
    const float c = speed * 0.666f;

    const float o1 = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
    const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
    const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
}

inline bool Spinner(const char* label, float radius, int thickness, const ImU32& color) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    // Render
    window->DrawList->PathClear();

    int num_segments = 30;
    int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

    const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

    const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

    for (int i = 0; i < num_segments; i++) {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + g.Time * 8) * radius,
            centre.y + ImSin(a + g.Time * 8) * radius));
    }

    window->DrawList->PathStroke(color, false, thickness);
}

// Dupe of DragFloatN with a tweak to add colored lines
inline bool DragFloatN_Colored(const char* label, float* v, int components, float v_speed, float v_min, float v_max, 
    const char* display_format = "%.3f", float power = 0, const ImU32* colors = nullptr)
{
    //ImGuiWindow* window = GetCurrentWindow();
    //if (window->SkipItems)
    //    return false;

    //ImGuiContext& g = *GImGui;
    //bool value_changed = false;
    //BeginGroup();
    //PushID(label);
    //PushMultiItemsWidths(components + 1, CalcItemWidth());

    //static const ImU32 s_colors[] = {
    //        0xBB0000FF, // red
    //        0xBB00FF00, // green
    //        0xBBFF0000, // blue
    //        0xBBFFFFFF, // white for alpha?
    //};

    //if (colors == nullptr)
    //{
    //    colors = s_colors;
    //}

    //for (int i = 0; i < components; i++)
    //{
    //    PushID(i);
    //    value_changed |= DragFloat("##v", &v[i], v_speed, v_min, v_max, display_format, power);

    //    const ImVec2 min = GetItemRectMin();
    //    const ImVec2 max = GetItemRectMax();
    //    const float spacing = g.Style.FrameRounding;
    //    const float halfSpacing = spacing / 2;

    //    // This is the main change
    //    window->DrawList->AddLine({ min.x + spacing, max.y - halfSpacing }, { max.x - spacing, max.y - halfSpacing }, colors[i], 4);

    //    SameLine(0, g.Style.ItemInnerSpacing.x);
    //    PopID();
    //    PopItemWidth();
    //}
    //PopID();

    //TextUnformatted(label, FindRenderedTextEnd(label));
    //EndGroup();

    //return value_changed;

    static const ImU32 s_colors[] = {
            0xBB0000FF, // red
            0xBB00FF00, // green
            0xBBFF0000, // blue
            0xBBFFFFFF, // white for alpha?
    };

    if (colors == nullptr)
    {
        colors = s_colors;
    }

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, CalcItemWidth());

    //sizeof(float),            "float", "%.3f","%f"
    size_t type_size = sizeof(float);
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= DragScalar("", ImGuiDataType_Float, v, v_speed, &v_min, &v_max, display_format, 0);

        const ImVec2 min = GetItemRectMin();
        const ImVec2 max = GetItemRectMax();
        const float spacing = g.Style.FrameRounding;
        const float halfSpacing = spacing / 2;

        // This is the main change
        window->DrawList->AddLine({ min.x + spacing, max.y - halfSpacing }, { max.x - spacing, max.y - halfSpacing }, colors[i], 4);

        PopID();
        PopItemWidth();
        v = (float*)((char*)v + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end)
    {
        SameLine(0, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}

inline void PushMultiItemsWidthsAndLabels(const char* labels[], int components, float w_full)
{
    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiStyle& style = GImGui->Style;
    if (w_full <= 0.0f)
        w_full = GetContentRegionAvail().x;

    const float w_item_one =
        ImMax(1.0f, (w_full - (style.ItemInnerSpacing.x * 2.0f) * (components - 1)) / (float)components) -
        style.ItemInnerSpacing.x;
    for (int i = 0; i < components; i++)
        window->DC.ItemWidthStack.push_back(w_item_one - CalcTextSize(labels[i]).x);
    window->DC.ItemWidth = window->DC.ItemWidthStack.back();
}

inline bool DragFloatNEx(const char* labels[], float* v, int components, float v_speed, float v_min, float v_max,
    const char* display_format = "%.3f", ImGuiSliderFlags flags = 0)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();

    PushMultiItemsWidthsAndLabels(labels, components, 0.0f);
    for (int i = 0; i < components; i++)
    {
        PushID(labels[i]);
        PushID(i);
        TextUnformatted(labels[i], FindRenderedTextEnd(labels[i]));
        SameLine();
        value_changed |= DragFloat("", &v[i], v_speed, v_min, v_max, display_format, flags | ImGuiSliderFlags_AlwaysClamp);
        SameLine(0, g.Style.ItemInnerSpacing.x);
        PopID();
        PopID();
        PopItemWidth();
    }

    EndGroup();

    return value_changed;
}

}