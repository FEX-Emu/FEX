#include "FEXImGui.h"
#include <imgui.h>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui_internal.h>
#include <vector>

namespace FEXImGui {
using namespace ImGui;

// FIXME: In principle this function should be called EndListBox(). We should rename it after re-evaluating if we want to keep the same signature.
bool ListBoxHeader(const char* label, int items_count)
{
    // Size default to hold ~7.25 items.
    // We add +25% worth of item height to allow the user to see at a glance if there are more items up/down, without looking at the scrollbar.
    // We don't add this extra bit if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.
    // I am expecting that someone will come and complain about this behavior in a remote future, then we can advise on a better solution.
    const ImGuiStyle& style = GetStyle();

    // We include ItemSpacing.y so that a list sized for the exact number of items doesn't make a scrollbar appears. We could also enforce that by passing a flag to BeginChild().
    ImVec2 size;
    size.x = 0.0f;

    auto Window = GetCurrentWindowRead();
    ImVec2 Base = Window->Rect().Min;
    ImVec2 Size = Window->Rect().Max;
    ImVec2 Height = Size - Base;
    size.y = Height.y - style.FramePadding.y * 2.0f;

    return ImGui::ListBoxHeader(label, size);
}

bool ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count)
{
    if (!ListBoxHeader(label, items_count))
        return false;

    // Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const bool item_selected = (i == *current_item);
            const char* item_text;
            if (!items_getter(data, i, &item_text))
                item_text = "*Unknown item*";

            PushID(i);
            if (Selectable(item_text, item_selected))
            {
                *current_item = i;
                value_changed = true;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }
    ListBoxFooter();
    if (value_changed)
        MarkItemEdited(g.CurrentWindow->DC.LastItemId);

    return value_changed;
}

bool CustomIRViewer(char const *buf, size_t buf_size, std::vector<IRLines> *lines) {
  ImGuiContext& g = *GImGui;
  const float inner_spacing = g.Style.ItemInnerSpacing.x;
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, 100.0f + inner_spacing * 2.0f);
  static float TextOffset = 0.0f;
  if (ImGui::BeginChildFrame(GetID(""), ImVec2(100, -1))) {
    const std::vector<ImVec4> LineColors = {
      ImVec4(1, 0, 0, 1),
      ImVec4(1, 1, 0, 1),
      ImVec4(0, 0, 0, 1),
      ImVec4(0, 0, 1, 1),
      ImVec4(1, 0, 1, 1),
      ImVec4(0, 1, 1, 1),
    };

    auto Window = GetCurrentWindowRead();
    auto DrawList = GetWindowDrawList();

    auto DrawIRLine = [&](size_t Index, float From, float To, ImVec4 Color) {
      ImVec2 Size = Window->Rect().Max;

      float LineWidth = 3.0;
      float LineSpacing = LineWidth * 5;

      // Zero indexing, add one
      Index = Index % LineColors.size();
      Index++;
      From -= TextOffset;
      To -= TextOffset;

      ImVec2 LeftFrom = ImVec2(Size.x - LineSpacing * static_cast<float>(Index), From);
      ImVec2 RightFrom = ImVec2(Size.x, From);
      ImVec2 LeftTo = ImVec2(Size.x - LineSpacing * static_cast<float>(Index), To);
      ImVec2 RightTo = ImVec2(Size.x, To);

      // We want to draw a three lines
      // One horizontal one from the "FROM" offset
      // One vertical one down the middle. Offset by Index
      // Another horizontol to the "TO" offset
      //
      // Additionally we want a triangle pointing to the TO location

      // Draw the from line
      DrawList->AddLine(LeftFrom, RightFrom, GetColorU32(Color), LineWidth);

      // Draw the to line
      DrawList->AddLine(LeftTo, RightTo, GetColorU32(Color), LineWidth);

      // Draw a small filled rectangle at the target
      DrawList->AddTriangleFilled(ImVec2(RightTo.x - LineWidth * 2, RightTo.y - LineWidth), ImVec2(RightTo.x - LineWidth * 2, RightTo.y + LineWidth), RightTo, GetColorU32(Color));

      // Draw the vertical line between the two
      DrawList->AddLine(LeftFrom, LeftTo, GetColorU32(Color), LineWidth);
    };

    ImVec2 Base = Window->Rect().Min;
    ImVec2 Size = Window->Rect().Max;
    DrawList->AddRectFilled(Base, Size, GetColorU32(ImVec4(0.5, 0.5, 0.5, 1.0)));

    float FontSize = Window->CalcFontSize();

    for (size_t i = 0; i < lines->size(); ++i) {
      float BaseOffset = Base.y + inner_spacing + FontSize / 2.0f;
      DrawIRLine(i, BaseOffset + static_cast<float>(lines->at(i).From) * FontSize, BaseOffset + static_cast<float>(lines->at(i).To) * FontSize, LineColors[i % LineColors.size()]);
    }
  }
  ImGui::EndChildFrame();

  ImGui::NextColumn();
  ImGui::InputTextMultiline("##IR", const_cast<char*>(buf), buf_size, ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);

  // This is a bit filthy
  // Pulls the window and if the child is available then we can pull its child and read the scroll amount
  // This lets the IR CFG lines match up with textbox, albiet a frame behind so it gets a bit of a wiggle
  auto win = GetCurrentWindow();
  if (win->DC.ChildWindows.size() == 2) {
    win = win->DC.ChildWindows[win->DC.ChildWindows.size() - 1];
    TextOffset = win->Scroll.y;
  }

  return false;
}


}
