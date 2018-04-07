// By Emil Ernerfeldt 2015-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
#pragma once

#include <cstdint>

namespace imgui_sw {

struct SwOptions
{
	bool bilinear_sample = true;
};

void bind_imgui_painting();
void paint_imgui(uint32_t* abgr, int width_pixels, int height_pixels, const SwOptions& options);
void unbind_imgui_painting();
bool show_options(SwOptions* io_options);
void show_stats();

} // namespace imgui_sw
