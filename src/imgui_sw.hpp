// By Emil Ernerfeldt 2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
#pragma once

#include <cstdint>

namespace imgui_sw {

struct SwOptions
{
	bool optimize_rectangles = true;
};

// Call once a the start of your program.
void bind_imgui_painting();

// The buffer is assumed to follow how ImGui packs pixels, i.e. ABGR by default.
// Change with IMGUI_USE_BGRA_PACKED_COLOR.
// If width/height differs from ImGui::GetIO().DisplaySize then
// the functions scales the UI to fit the given pixel buffer.
void paint_imgui(uint32_t* pixels, int width_pixels, int height_pixels, const SwOptions& options);

// All to free resources.
void unbind_imgui_painting();

// Show ImGui controls for rendering options if you want to.
bool show_options(SwOptions* io_options);

// Show rendering stats in an ImGui window if you want to.
void show_stats();

} // namespace imgui_sw
