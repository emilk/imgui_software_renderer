// By Emil Ernerfeldt 2015-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
#include "imgui_sw.hpp"

#include <cmath>
#include <vector>

#include <imgui/imgui.h>
#include <loguru.hpp>

namespace imgui_sw {
namespace {

struct Stats
{
	double uniform_triangle_pixels   = 0;
	double textured_triangle_pixels  = 0;
	double rest_triangle_pixels      = 0;
	double uniform_rectangle_pixels  = 0;
	double textured_rectangle_pixels = 0;
};

struct Texture
{
	std::vector<uint8_t> pixels;
	int                  width;
	int                  height;
};

struct PaintTarget
{
	uint32_t* pixels;
	int       width;
	int       height;
	ImVec2    scale; // Multiply ImGui coordinates with this to get pixel coordinates.
};

float min3(float a, float b, float c)
{
	if (a < b && a < c) { return a; }
	return b < c ? b : c;
}

float max3(float a, float b, float c)
{
	if (a > b && a > c) { return a; }
	return b > c ? b : c;
}

/*
struct ImDrawVert
{
	ImVec2  pos;
	ImVec2  uv;
	ImU32   col;
};
*/

inline float edgeFunction(const ImVec2& a, const ImVec2& b, const ImVec2& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

inline ImVec2 operator*(const float f, const ImVec2& v)
{
	return ImVec2{f * v.x, f * v.y};
}

inline ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
	return ImVec2{a.x + b.x, a.y + b.y};
}

inline bool operator==(const ImVec2& a, const ImVec2& b)
{
	return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const ImVec2& a, const ImVec2& b)
{
	return a.x != b.x || a.y != b.y;
}

inline ImVec4 operator*(const float f, const ImVec4& v)
{
	return ImVec4{f * v.x, f * v.y, f * v.z, f * v.w};
}

inline ImVec4 operator+(const ImVec4& a, const ImVec4& b)
{
	return ImVec4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

float sample_texture(const Texture& texture, float x, float y)
{
	const auto xmin = 0.f;
	const auto ymin = 0.f;

	const auto xmax = static_cast<float>(texture.width  - 1);
	const auto ymax = static_cast<float>(texture.height - 1);

	auto x1 = std::floor(x);
	auto y1 = std::floor(y);

	auto x2 = x1 + 1;
	auto y2 = y1 + 1;

	const auto u1 = x2 - x;
	const auto v1 = y2 - y;

	const auto u2 = x - x1;
	const auto v2 = y - y1;

	// Clamp:
	if (x1 < xmin) {
		x1 = xmin;
		x2 = xmin;
	}
	if (x2 > xmax) {
		x1 = xmax;
		x2 = xmax;
	}
	if (y1 < ymin) {
		y1 = ymin;
		y2 = ymin;
	}
	if (y2 > ymax) {
		y1 = ymax;
		y2 = ymax;
	}

	const auto xi1 = static_cast<size_t>(x1);
	const auto yi1 = static_cast<size_t>(y1);

	const auto xi2 = static_cast<size_t>(x2);
	const auto yi2 = static_cast<size_t>(y2);

	const auto i11 = texture.pixels[xi1 + yi1 * texture.width] / 255.0f;
	const auto i12 = texture.pixels[xi1 + yi2 * texture.width] / 255.0f;
	const auto i21 = texture.pixels[xi2 + yi1 * texture.width] / 255.0f;
	const auto i22 = texture.pixels[xi2 + yi2 * texture.width] / 255.0f;

	return
		u1 * v1 * i11 +
		u1 * v2 * i12 +
		u2 * v1 * i21 +
		u2 * v2 * i22;
}

struct ColorInt
{
	uint32_t a, b, g, r;

	ColorInt() = default;

	explicit ColorInt(uint32_t x)
	{
		a = (x >> IM_COL32_A_SHIFT) & 0xFFu;
		b = (x >> IM_COL32_B_SHIFT) & 0xFFu;
		g = (x >> IM_COL32_G_SHIFT) & 0xFFu;
		r = (x >> IM_COL32_R_SHIFT) & 0xFFu;
	}

	uint32_t toUint32() const
	{
		return (a << 24u) | (b << 16u) | (g << 8u) | r;
	}
};

ImVec4 color_convert_u32_to_float4(ImU32 in)
{
	const float s = 1.0f / 255.0f;
	return ImVec4(
		((in >> IM_COL32_R_SHIFT) & 0xFF) * s,
		((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
		((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
		((in >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

ImU32 color_convert_float4_to_u32(const ImVec4& in)
{
	ImU32 out;
	out  = uint32_t(in.x * 255.0f + 0.5f) << IM_COL32_R_SHIFT;
	out |= uint32_t(in.y * 255.0f + 0.5f) << IM_COL32_G_SHIFT;
	out |= uint32_t(in.z * 255.0f + 0.5f) << IM_COL32_B_SHIFT;
	out |= uint32_t(in.w * 255.0f + 0.5f) << IM_COL32_A_SHIFT;
	return out;
}

inline ColorInt blend(ColorInt target, ColorInt source)
{
	ColorInt result;
	result.a = source.a; // Whatever.
	result.b = (source.b * source.a + target.b * (255 - source.a)) / 255;
	result.g = (source.g * source.a + target.g * (255 - source.a)) / 255;
	result.r = (source.r * source.a + target.r * (255 - source.a)) / 255;
	return result;
}

void paint_uniform_rectangle(
	const PaintTarget& target,
	const ImVec2&      min_f,
	const ImVec2&      max_f,
	const ColorInt&        color)
{
	// Inclusive integer bounding box:
	int min_x_i = static_cast<int>(target.scale.x * min_f.x);
	int min_y_i = static_cast<int>(target.scale.y * min_f.y);
	int max_x_i = static_cast<int>(target.scale.x * max_f.x);
	int max_y_i = static_cast<int>(target.scale.y * max_f.y);

	// Clamp to render target:
	min_x_i = std::max(min_x_i, 0);
	min_y_i = std::max(min_y_i, 0);
	max_x_i = std::min(max_x_i, target.width - 1);
	max_y_i = std::min(max_y_i, target.height - 1);

	for (int y = min_y_i; y <= max_y_i; ++y) {
		for (int x = min_x_i; x <= max_x_i; ++x) {
			uint32_t& target_pixel = target.pixels[y * target.width + x];
			target_pixel = blend(ColorInt(target_pixel), color).toUint32();
		}
	}
}

void paint_triangle(
	const PaintTarget& target,
	const Texture&     texture,
	const ImVec4&      clip_rect,
	const ImDrawVert&  vert_0,
	const ImDrawVert&  vert_1,
	const ImDrawVert&  vert_2,
	const SwOptions&   options,
	Stats*             stats)
{
	CHECK_GT_F(texture.width, 0);
	CHECK_GT_F(texture.height, 0);

	ImVec2 v0 = ImVec2(target.scale.x * vert_0.pos.x, target.scale.y * vert_0.pos.y);
	ImVec2 v1 = ImVec2(target.scale.x * vert_1.pos.x, target.scale.y * vert_1.pos.y);
	ImVec2 v2 = ImVec2(target.scale.x * vert_2.pos.x, target.scale.y * vert_2.pos.y);

	// Find bounding box:
	float min_x_f = min3(v0.x, v1.x, v2.x);
	float min_y_f = min3(v0.y, v1.y, v2.y);
	float max_x_f = max3(v0.x, v1.x, v2.x);
	float max_y_f = max3(v0.y, v1.y, v2.y);

	// Clamp to clip_rect:
	min_x_f = std::max(min_x_f, target.scale.x * clip_rect.x);
	min_y_f = std::max(min_y_f, target.scale.y * clip_rect.y);
	max_x_f = std::min(max_x_f, target.scale.x * clip_rect.z);
	max_y_f = std::min(max_y_f, target.scale.y * clip_rect.w);

	// Inclusive integer bounding box:
	int min_x_i = static_cast<int>(min_x_f);
	int min_y_i = static_cast<int>(min_y_f);
	int max_x_i = static_cast<int>(max_x_f);
	int max_y_i = static_cast<int>(max_y_f);

	// Clamp to render target:
	min_x_i = std::max(min_x_i, 0);
	min_y_i = std::max(min_y_i, 0);
	max_x_i = std::min(max_x_i, target.width - 1);
	max_y_i = std::min(max_y_i, target.height - 1);

	const bool uniform_color = (vert_0.col == vert_1.col && vert_0.col == vert_2.col);
	const bool has_texture = (vert_0.uv != vert_1.uv || vert_0.uv != vert_2.uv);

	const float determinant = edgeFunction(v0, v1, v2);
	if (determinant == 0.0f) { return; }
	// if (determinant >= 0.0f) { return; } // Backface culling?
	const auto inv_det = 1.0f / determinant;

	const auto num_pixels = std::abs(determinant / 2.0f);

	if (uniform_color && !has_texture) {
		stats->uniform_triangle_pixels += num_pixels;
	} else if (has_texture) {
		stats->textured_triangle_pixels += num_pixels;
	} else {
		stats->rest_triangle_pixels += num_pixels;
	}

	ImVec4 c0 = color_convert_u32_to_float4(vert_0.col);
	ImVec4 c1 = color_convert_u32_to_float4(vert_1.col);
	ImVec4 c2 = color_convert_u32_to_float4(vert_2.col);

	for (int y = min_y_i; y <= max_y_i; ++y) {
		for (int x = min_x_i; x <= max_x_i; ++x) {
			const ImVec2 p = ImVec2(x, y);
			const float w0 = inv_det * edgeFunction(v1, v2, p);
			const float w1 = inv_det * edgeFunction(v2, v0, p);
			const float w2 = inv_det * edgeFunction(v0, v1, p);
			if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) { continue; }

			uint32_t& target_pixel = target.pixels[y * target.width + x];

			if (uniform_color && !has_texture) {
				target_pixel = blend(ColorInt(target_pixel), ColorInt(vert_0.col)).toUint32();
				continue;
			}

			ImVec4 src_color;

			if (uniform_color) {
				src_color = color_convert_u32_to_float4(vert_0.col);
			} else {
				src_color = w0 * c0 + w1 * c1 + w2 * c2;
			}

			if (has_texture) {
				ImVec2 uv = w0 * vert_0.uv + w1 * vert_1.uv + w2 * vert_2.uv;

				if (options.bilinear_sample) {
					const float tx = uv.x * texture.width;
					const float ty = uv.y * texture.height;
					src_color.w = sample_texture(texture, tx, ty);
				} else {
					int tx = uv.x * (texture.width - 1.0f)  + 0.5f;
					int ty = uv.y * (texture.height - 1.0f) + 0.5f;
					DCHECK_GE_F(tx, 0);
					DCHECK_GE_F(ty, 0);
					DCHECK_LT_F(tx, texture.width);
					DCHECK_LT_F(ty, texture.height);
					const uint8_t texel = texture.pixels[ty * texture.width + tx];
					src_color.w *= texel / 255.0f;
				}
			}

			if (src_color.w <= 0.0f) { continue; }
			if (src_color.w >= 1.0f) {
				target_pixel = color_convert_float4_to_u32(src_color);
				continue;
			}

			ImVec4 target_color = color_convert_u32_to_float4(target_pixel);
			const auto blended_color = src_color.w * src_color + (1.0f - src_color.w) * target_color;
			target_pixel = color_convert_float4_to_u32(blended_color);
		}
	}
}

void paint_draw_list(const PaintTarget& target, const ImDrawList* cmd_list, const SwOptions& options, Stats* stats)
{
	const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer[0];

	const ImDrawVert* vertices = cmd_list->VtxBuffer.Data;
	const auto num_verts = cmd_list->VtxBuffer.size();

	for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
	{
		const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
		if (pcmd->UserCallback) {
			pcmd->UserCallback(cmd_list, pcmd);
		} else {
			const auto texture = reinterpret_cast<const Texture*>(pcmd->TextureId);
			CHECK_NOTNULL_F(texture);
			for (int i = 0; i + 3 <= pcmd->ElemCount; ) {
				CHECK_LT_F(idx_buffer[i + 0], num_verts);
				CHECK_LT_F(idx_buffer[i + 1], num_verts);
				CHECK_LT_F(idx_buffer[i + 2], num_verts);

				const auto idx_0 = idx_buffer[i + 0];
				const auto idx_1 = idx_buffer[i + 1];
				const auto idx_2 = idx_buffer[i + 2];

				const ImDrawVert& v0 = vertices[idx_0];
				const ImDrawVert& v1 = vertices[idx_1];
				const ImDrawVert& v2 = vertices[idx_2];

				// A lot of the big stuff are uniformly colored rectangles,
				// so we can save a lot of CPU by detecting them:
				if (options.optimize_rectangles && i + 6 <= pcmd->ElemCount) {
					const auto idx_3 = idx_buffer[i + 3];
					const auto idx_4 = idx_buffer[i + 4];
					const auto idx_5 = idx_buffer[i + 5];

					if (idx_0 == idx_3 && idx_2 == idx_4)
					{
						const ImDrawVert& v3 = vertices[idx_3];
						const ImDrawVert& v4 = vertices[idx_4];
						const ImDrawVert& v5 = vertices[idx_5];

						const bool has_uniform_color =
							v0.col == v1.col &&
							v0.col == v2.col &&
							v0.col == v3.col &&
							v0.col == v4.col &&
							v0.col == v5.col;

						if (has_uniform_color) {
							ImVec2 min = v0.pos;
							ImVec2 max = v4.pos;

							const bool is_rect =
								v0.pos == ImVec2(min.x, min.y) &&
								v1.pos == ImVec2(max.x, min.y) &&
								v2.pos == ImVec2(max.x, max.y) &&
								v3.pos == ImVec2(min.x, min.y) &&
								v4.pos == ImVec2(max.x, max.y) &&
								v5.pos == ImVec2(min.x, max.y);

							const bool has_uniform_uv =
								v0.uv  == v1.uv  &&
								v0.uv  == v2.uv  &&
								v0.uv  == v3.uv  &&
								v0.uv  == v4.uv  &&
								v0.uv  == v5.uv;

							if (is_rect) {
								min.x = std::max(min.x, pcmd->ClipRect.x);
								min.y = std::max(min.y, pcmd->ClipRect.y);
								max.x = std::min(max.x, pcmd->ClipRect.z);
								max.y = std::min(max.y, pcmd->ClipRect.w);
								const auto num_pixels = (max.x - min.x) * (max.y - min.y);
								if (has_uniform_uv) {
									paint_uniform_rectangle(target, min, max, ColorInt(v0.col));
									stats->uniform_rectangle_pixels += num_pixels;
									i += 6;
									continue;
								} else {
									// TODO: optimize textured triangles
									stats->textured_rectangle_pixels += num_pixels;
								}
							}
						}
					}
				}

				paint_triangle(target, *texture, pcmd->ClipRect, v0, v1, v2, options, stats);
				i += 3;
			}
		}
		idx_buffer += pcmd->ElemCount;
	}
}

} // namespace

void make_style_fast()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.AntiAliasedLines = false;
	style.AntiAliasedFill = false;
	style.WindowRounding = 0;
}

void bind_imgui_painting()
{
	ImGuiIO& io = ImGui::GetIO();

	// Load default font (embedded in code):
	unsigned char* tex_data;
	int font_width, font_height;
	io.Fonts->GetTexDataAsAlpha8(&tex_data, &font_width, &font_height);
	const auto texture = new Texture{
		std::vector<uint8_t>(tex_data, tex_data + font_width * font_height),
		font_width,
		font_height
	};

	io.Fonts->TexID = texture;

	make_style_fast();
}

Stats s_stats;

void paint_imgui(uint32_t* pixels, int width_pixels, int height_pixels, const SwOptions& options)
{
	const float width_points = ImGui::GetIO().DisplaySize.x;
	const float height_points = ImGui::GetIO().DisplaySize.y;
	const ImVec2 scale{width_pixels / width_points, height_pixels / height_points};
	PaintTarget target{pixels, width_pixels, height_pixels, scale};
	const ImDrawData* draw_data = ImGui::GetDrawData();

	s_stats = Stats{};
	for (int i = 0; i < draw_data->CmdListsCount; ++i) {
		paint_draw_list(target, draw_data->CmdLists[i], options, &s_stats);
	}
}

void unbind_imgui_painting()
{
	ImGuiIO& io = ImGui::GetIO();
	delete reinterpret_cast<Texture*>(io.Fonts->TexID);
	io.Fonts = nullptr;
}

bool show_options(SwOptions* io_options)
{
	CHECK_NOTNULL_F(io_options);
	bool changed = false;
	changed |= ImGui::Checkbox("optimize_rectangles", &io_options->optimize_rectangles);
	changed |= ImGui::Checkbox("bilinear_sample",     &io_options->bilinear_sample);
	return changed;
}

void show_stats()
{
	ImGui::Text("uniform_triangle_pixels:  %7.0f", s_stats.uniform_triangle_pixels);
	ImGui::Text("textured_triangle_pixels: %7.0f", s_stats.textured_triangle_pixels);
	ImGui::Text("rest_triangle_pixels:     %7.0f", s_stats.rest_triangle_pixels);
	ImGui::Text("uniform_rectangle_pixels: %7.0f", s_stats.uniform_rectangle_pixels);
	ImGui::Text("textred_rectangle_pixels: %7.0f", s_stats.textured_rectangle_pixels);
}

} // namespace imgui_sw
