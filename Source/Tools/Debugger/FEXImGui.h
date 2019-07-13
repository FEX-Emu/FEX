#pragma once
#include <imgui.h>
#include <cstdint>
#include <vector>

namespace FEXImGui {

// Listbox that fills the child window
bool ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count);

struct IRLines {
  size_t From;
  size_t To;
};

bool CustomIRViewer(char const *buf, size_t buf_size, std::vector<IRLines> *lines);

}
