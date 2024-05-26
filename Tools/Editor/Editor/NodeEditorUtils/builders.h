//------------------------------------------------------------------------------
// LICENSE
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
//
// CREDITS
//   Written by Michal Cichon
//------------------------------------------------------------------------------
# pragma once


//------------------------------------------------------------------------------
#include "imgui-node-editor/imgui_node_editor.h"

#include <vector>


//------------------------------------------------------------------------------
namespace ax {
namespace NodeEditor {
namespace Utilities {


//------------------------------------------------------------------------------
struct BlueprintNodeBuilder
{
    enum BG_DRAW_TYPE
    {
        SEPARATOR
    };

    struct BgDrawCall
    {
        BG_DRAW_TYPE type;
        uint8_t payload[32];
        uint16_t payloadCount = 0;

        template <typename T>
        T& NextVar()
        {
            assert(payloadCount < sizeof(payload));
            auto ret = (T*)&payload[payloadCount];
            payloadCount += sizeof(T);
            return *ret;
        }
    };

    BlueprintNodeBuilder(ImTextureID texture = nullptr, int textureWidth = 0, int textureHeight = 0);

    void Begin(NodeId id);
    void End();

    void Header(const ImVec4& color = ImColor(128, 195, 255));
    void EndHeader();

    void Input(PinId id);
    void EndInput();

    void Middle();

    void Output(PinId id);
    void EndOutput();

    void Separator();

private:
    void SeparatorImpl(ImDrawList* drawList, BgDrawCall* call);

    enum class Stage
    {
        Invalid,
        Begin,
        Header,
        Content,
        Input,
        Output,
        Middle,
        End
    };

    bool SetStage(Stage stage);

    void Pin(PinId id, ax::NodeEditor::PinKind kind);
    void EndPin();

    ImTextureID HeaderTextureId;
    int         HeaderTextureWidth;
    int         HeaderTextureHeight;
    NodeId      CurrentNodeId;
    Stage       CurrentStage;
    ImU32       HeaderColor;
    ImVec2      NodeMin;
    ImVec2      NodeMax;
    ImVec2      HeaderMin;
    ImVec2      HeaderMax;
    ImVec2      ContentMin;
    ImVec2      ContentMax;
    bool        HasHeader;

    std::vector<BgDrawCall> bgDrawCalls;
};



//------------------------------------------------------------------------------
} // namespace Utilities

bool BeginNodeCombo(const char* label, const char* preview_value, ImGuiComboFlags flags);
void EndNodeCombo();

} // namespace Editor
} // namespace ax