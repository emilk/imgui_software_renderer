#include <SDL2/SDL.h>

#include <emilib/imgui_helpers.hpp>
#include <emilib/imgui_sdl.hpp>
#include <emilib/timer.hpp>
#include <loguru.hpp>

// For gl path:
#include <emilib/gl_lib_opengl.hpp>
#include <emilib/gl_lib_sdl.hpp>
#include <emilib/imgui_gl_lib.hpp>

#include "imgui_sw.hpp"

using namespace emilib;

void customRendering(ImVec4 col)
{
	// Taken from imgui_demo.cpp:
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	static float sz = 36.0f;
	static float thickness = 4.0f;
	const ImVec2 p = ImGui::GetCursorScreenPos();
	const ImU32 col32 = ImColor(col);
	float x = p.x + 4.0f, y = p.y + 4.0f, spacing = 8.0f;
	for (int n = 0; n < 2; n++) {
		float curr_thickness = (n == 0) ? 1.0f : thickness;
		draw_list->AddCircle(ImVec2(x+sz*0.5f, y+sz*0.5f), sz*0.5f, col32, 20, curr_thickness); x += sz+spacing;
		draw_list->AddRect(ImVec2(x, y), ImVec2(x+sz, y+sz), col32, 0.0f, ImDrawCornerFlags_All, curr_thickness); x += sz+spacing;
		draw_list->AddRect(ImVec2(x, y), ImVec2(x+sz, y+sz), col32, 10.0f, ImDrawCornerFlags_All, curr_thickness); x += sz+spacing;
		draw_list->AddRect(ImVec2(x, y), ImVec2(x+sz, y+sz), col32, 10.0f, ImDrawCornerFlags_TopLeft|ImDrawCornerFlags_BotRight, curr_thickness); x += sz+spacing;
		draw_list->AddTriangle(ImVec2(x+sz*0.5f, y), ImVec2(x+sz,y+sz-0.5f), ImVec2(x,y+sz-0.5f), col32, curr_thickness); x += sz+spacing;
		draw_list->AddLine(ImVec2(x, y), ImVec2(x+sz, y   ), col32, curr_thickness); x += sz+spacing; // Horizontal line (note: drawing a filled rectangle will be faster!)
		draw_list->AddLine(ImVec2(x, y), ImVec2(x,    y+sz), col32, curr_thickness); x += spacing;    // Vertical line (note: drawing a filled rectangle will be faster!)
		draw_list->AddLine(ImVec2(x, y), ImVec2(x+sz, y+sz), col32, curr_thickness); x += sz+spacing; // Diagonal line
		draw_list->AddBezierCurve(ImVec2(x, y), ImVec2(x+sz*1.3f,y+sz*0.3f), ImVec2(x+sz-sz*1.3f,y+sz-sz*0.3f), ImVec2(x+sz, y+sz), col32, curr_thickness);
		x = p.x + 4;
		y += sz+spacing;
	}
	draw_list->AddCircleFilled(ImVec2(x+sz*0.5f, y+sz*0.5f), sz*0.5f, col32, 32); x += sz+spacing;
	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x+sz, y+sz), col32); x += sz+spacing;
	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x+sz, y+sz), col32, 10.0f); x += sz+spacing;
	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x+sz, y+sz), col32, 10.0f, ImDrawCornerFlags_TopLeft|ImDrawCornerFlags_BotRight); x += sz+spacing;
	draw_list->AddTriangleFilled(ImVec2(x+sz*0.5f, y), ImVec2(x+sz,y+sz-0.5f), ImVec2(x,y+sz-0.5f), col32); x += sz+spacing;
	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x+sz, y+thickness), col32); x += sz+spacing;          // Horizontal line (faster than AddLine, but only handle integer thickness)
	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x+thickness, y+sz), col32); x += spacing+spacing;     // Vertical line (faster than AddLine, but only handle integer thickness)
	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x+1, y+1), col32);          x += sz;                  // Pixel (faster than AddLine)
	draw_list->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x+sz, y+sz), IM_COL32(0,0,0,255), IM_COL32(255,0,0,255), IM_COL32(255,255,0,255), IM_COL32(0,255,0,255));
	ImGui::Dummy(ImVec2((sz+spacing)*8, (sz+spacing)*3));
}

void showTestWindows()
{
	static ImVec4 s_some_color{ 0.7f, 0.8f, 0.9f, 0.5f };

	ImGui::SetNextWindowPos(ImVec2{700.0f, 32.0f});
	ImGui::SetNextWindowSize(ImVec2{400.0f, 800.0f});
	if (ImGui::Begin("Graphics")) {
		ImGui::ColorPicker4("some color", &s_some_color.x, ImGuiColorEditFlags_PickerHueBar);
		ImGui::ColorPicker4("same color", &s_some_color.x, ImGuiColorEditFlags_PickerHueWheel);
		customRendering(s_some_color);
	}
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2{32.0f, 400.0f});
	ImGui::SetNextWindowSize(ImVec2{600.0f, 600.0f});
	if (ImGui::Begin("Test")) {
		for (int i = 100; i > 0; --i) {
			ImGui::Text("%d bottles of beer on the wall, %d bottles of beer. Take one down, pass it around, %d bottles of beer on the wall.", i, i, i - 1);
		}
	}
	ImGui::End();
}

void run_software()
{
	int width_points = 1280;
	int height_points = 1024;

	CHECK_EQ_F(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO), 0, "SDL_Init fail: %s\n", SDL_GetError());
	const auto window = SDL_CreateWindow("ImGui Software Renderer Example", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, width_points, height_points, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	CHECK_NOTNULL_F(window, "Failed to create window: %s", SDL_GetError());

	SDL_GetWindowSize(window, &width_points, &height_points);

	int width_pixels, height_pixels;
	SDL_GL_GetDrawableSize(window, &width_pixels, &height_pixels);

	const auto pixels_per_point = static_cast<float>(width_pixels) / static_cast<float>(width_points);

	LOG_F(INFO, "%dx%d points (%dx%d pixels)", width_points, height_points, width_pixels, height_pixels);

	IMGUI_CHECKVERSION();
	ImGui_SDL imgui_sdl(width_points, height_points, pixels_per_point);

	auto surface = SDL_GetWindowSurface(window);
	auto renderer = SDL_CreateSoftwareRenderer(surface);
	CHECK_NOTNULL_F(renderer, "Failed to create software renderer: %s", SDL_GetError());

	SDL_Texture* texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, width_pixels, height_pixels);
	CHECK_NOTNULL_F(texture);

	std::vector<uint32_t> pixel_buffer(width_pixels * height_pixels, 0);
	std::vector<uint32_t> point_buffer(width_points * height_points, 0);

	imgui_sw::bind_imgui_painting();

	imgui_sw::SwOptions sw_options;
	bool full_res = (width_pixels == width_points);
	bool fast_style = false;

	double paint_time = 0;
	double upsample_time = 0;

	bool quit = false;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) { quit = true; }
			imgui_sdl.on_event(event);
		}
		imgui_sdl.new_frame();

		// --------------------------------------------------------------------

		if (ImGui::BeginMainMenuBar()) {
			imgui_helpers::show_im_gui_menu();
			ImGui::EndMainMenuBar();
		}

		ImGui::SetNextWindowPos(ImVec2{32.0f, 32.0f});
		if (ImGui::Begin("Settings")) {
			if (ImGui::Checkbox("Tweak ImGui style for speed", &fast_style)) {
				if (fast_style) {
					imgui_sw::make_style_fast();
				} else {
					imgui_sw::restore_style();
				}
			}

			ImGui::Checkbox("full_res", &full_res);
			ImGui::Text("Paint time: %.2f ms", 1000 * paint_time);
			if (!full_res) {
				ImGui::Text("Upsample time: %.2f ms", 1000 * paint_time);
			}
			imgui_sw::show_options(&sw_options);
			imgui_sw::show_stats();
		}
		ImGui::End();

		showTestWindows();

		// --------------------------------------------------------------------

		imgui_sdl.paint();
		double frame_paint_time;

		if (full_res) {
			std::fill_n(pixel_buffer.data(), pixel_buffer.size(), 0x19191919u);
			Timer paint_timer;
			paint_imgui(pixel_buffer.data(), width_pixels, height_pixels, sw_options);
			frame_paint_time = paint_timer.secs();
		} else {
			// Render ImGui in low resolution:
			CHECK_LE_F(width_points, width_pixels);
			std::fill_n(point_buffer.data(), point_buffer.size(), 0x19191919u);
			Timer paint_timer;
			paint_imgui(point_buffer.data(), width_points, height_points, sw_options);
			frame_paint_time = paint_timer.secs();

			// Now upsample it (TODO: a faster way).
			Timer upsample_timer;
			const int scale = height_pixels / height_points;
			for (int y_px = 0; y_px < height_pixels; ++y_px) {
				const auto y_pts = y_px / scale;
				for (int x_px = 0; x_px < width_pixels; ++x_px) {
					const auto x_pts = x_px / scale;
					pixel_buffer[y_px * width_pixels + x_px] = point_buffer[y_pts * width_points + x_pts];
				}
			}
			upsample_time = upsample_timer.secs();
		}

		paint_time = 0.95 * paint_time + 0.05 * frame_paint_time;

		SDL_UpdateTexture(texture, nullptr, pixel_buffer.data(), width_pixels * sizeof(Uint32));
		// SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
	}
}

void run_gl()
{
	sdl::Params sdl_params;
	sdl_params.window_name = "ImGui GL Renderer";
	sdl_params.width_points = 1280;
	sdl_params.height_points = 1024;
	auto sdl = sdl::init(sdl_params);
	ImGui_SDL imgui_sdl(sdl.width_points, sdl.height_points, sdl.pixels_per_point);
	gl::bind_imgui_painting();

	double paint_time = 0;
	bool wireframe = false;

	bool quit = false;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) { quit = true; }
			imgui_sdl.on_event(event);
		}
		imgui_sdl.new_frame();

		if (ImGui::BeginMainMenuBar()) {
			imgui_helpers::show_im_gui_menu();
			ImGui::EndMainMenuBar();
		}
		ImGui::Text("Paint time: %.2f ms", 1000 * paint_time);
		ImGui::Checkbox("wireframe", &wireframe);

		showTestWindows();

		imgui_sdl.paint();

		glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
		Timer paint_timer;
		gl::paint_imgui();
		paint_time = paint_timer.secs();
		SDL_GL_SwapWindow(sdl.window);
	}
}

int main(int argc, char* argv[])
{
	loguru::g_colorlogtostderr = false;
	loguru::init(argc, argv);

	run_software();
	// run_gl(); // Reference renderer.
}
