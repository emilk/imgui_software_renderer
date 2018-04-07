#include <SDL2/SDL.h>

#include <emilib/imgui_helpers.hpp>
#include <emilib/imgui_sdl.hpp>
#include <emilib/timer.hpp>
#include <loguru.hpp>

#include "imgui_sw.hpp"

using namespace emilib;

int main(int argc, char* argv[])
{
	loguru::g_colorlogtostderr = false;
	loguru::init(argc, argv);

	int width_points = 1024;
	int height_points = 768;

	CHECK_EQ_F(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO), 0, "SDL_Init fail: %s\n", SDL_GetError());

	int window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;

#if TARGET_OS_IPHONE
	window_flags |= SDL_WINDOW_BORDERLESS; // Hides the status bar
#else
	// window_flags |= SDL_WINDOW_RESIZABLE;
#endif

	const auto window = SDL_CreateWindow("ImGui Software renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width_points, height_points, window_flags);
	CHECK_NOTNULL_F(window, "Failed to create software renderer: %s", SDL_GetError());

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
	std::vector<uint32_t> pixels(width_pixels * height_pixels, 0);

	imgui_sw::bind_imgui_painting();

	imgui_sw::SwOptions sw_options;

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
		imgui_sw::show_options(&sw_options);
		imgui_sw::show_stats();
		imgui_sdl.paint();

		memset(pixels.data(), 0, pixels.size() * 4);
		std::fill_n(pixels.data(), width_pixels * height_pixels, 0x44444444u);

		Timer paint_timer;
		paint_imgui(pixels.data(), width_pixels, height_pixels, sw_options);
		paint_time = paint_timer.secs();

		SDL_UpdateTexture(texture, nullptr, pixels.data(), width_pixels * sizeof(Uint32));
		// SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
	}
}
