#pragma once
#include "imgui.h"
#include <filesystem>

namespace amphetaminehaxx {
void initialize(const std::filesystem::path& directory);
void render();
void set_visible(bool show);
bool excludes_capture();
extern bool vsync;
extern bool drag_moves_window;
extern ImVec2 menu_pos;
extern ImVec2 menu_size;
}
