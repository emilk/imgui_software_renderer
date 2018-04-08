#include <SDL2/SDL.h>

#include <emilib/imgui_helpers.hpp>
#include <emilib/imgui_sdl.hpp>
#include <emilib/irange.hpp>
#include <emilib/timer.hpp>
#include <loguru.hpp>

// For gl path:
#include <emilib/gl_lib_opengl.hpp>
#include <emilib/gl_lib_sdl.hpp>
#include <emilib/imgui_gl_lib.hpp>

#include "imgui_sw.hpp"

using namespace emilib;

void run_software()
{
	int width_points = 1280;
	int height_points = 1024;

	CHECK_EQ_F(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO), 0, "SDL_Init fail: %s\n", SDL_GetError());

	int window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	// window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;

#if TARGET_OS_IPHONE
	window_flags |= SDL_WINDOW_BORDERLESS; // Hides the status bar
#else
	// window_flags |= SDL_WINDOW_RESIZABLE;
#endif

	const auto window = SDL_CreateWindow("ImGui Software Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width_points, height_points, window_flags);
	CHECK_NOTNULL_F(window, "Failed to create window: %s", SDL_GetError());

	SDL_GetWindowSize(window, &width_points, &height_points);

	int width_pixels, height_pixels;
	SDL_GL_GetDrawableSize(window, &width_pixels, &height_pixels);

	const auto pixels_per_point = (float)width_pixels / (float)width_points;

	LOG_F(INFO, "%dx%d points (%dx%d pixels)", width_points, height_points, width_pixels, height_pixels);

	ImGui_SDL imgui_sdl(width_points, height_points, pixels_per_point);

	auto surface = SDL_GetWindowSurface(window);
	auto renderer = SDL_CreateSoftwareRenderer(surface);
	CHECK_NOTNULL_F(renderer, "Failed to create software renderer: %s", SDL_GetError());

	SDL_Texture* texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, width_pixels, height_pixels);
	CHECK_NOTNULL_F(texture);

	bool full_res = (width_pixels == width_points);

	std::vector<uint32_t> pixel_buffer(width_pixels * height_pixels, 0);
	std::vector<uint32_t> point_buffer(width_points * height_points, 0);

	imgui_sw::bind_imgui_painting();

	imgui_sw::SwOptions sw_options;

	double paint_time = 0;
	double rescale_time = 0;

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

		if (ImGui::Button("Tweak ImGui style for speed")) {
			imgui_sw::make_style_fast();
		}

		ImGui::Checkbox("full_res", &full_res);
		ImGui::Text("Paint time: %.2f ms", 1000 * paint_time);
		if (!full_res) {
			ImGui::Text("Rescale time: %.2f ms", 1000 * paint_time);
		}
		imgui_sw::show_options(&sw_options);
		imgui_sw::show_stats();
		imgui_sdl.paint();

		double frame_paint_time;

		if (full_res) {
			std::fill_n(pixel_buffer.data(), pixel_buffer.size(), 0x19191919u);
			Timer paint_timer;
			paint_imgui(pixel_buffer.data(), width_pixels, height_pixels, sw_options);
			frame_paint_time = paint_timer.secs();
		} else {
			CHECK_LT_F(width_points, width_pixels);
			std::fill_n(point_buffer.data(), point_buffer.size(), 0x19191919u);
			Timer paint_timer;
			paint_imgui(point_buffer.data(), width_points, height_points, sw_options);
			frame_paint_time = paint_timer.secs();

			Timer rescale_timer;
			const auto scale = height_pixels / height_points;
			for (const auto y_px : irange(0, height_pixels)) {
				const auto y_pts = y_px / scale;
				for (const auto x_px : irange(0, width_pixels)) {
					const auto x_pts = x_px / scale;
					pixel_buffer[y_px * width_pixels + x_px] = point_buffer[y_pts * width_points + x_pts];
				}
			}
			rescale_time = rescale_timer.secs();
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
		imgui_sdl.paint();

		glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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
	// run_gl();
}
