# Dear ImGui software renderer
This is a software renderer for [Dear ImGui](https://github.com/ocornut/imgui).
I built it not out of a specific need, but because it was fun.
The goal was to get something accurate and decently fast in not too many lines of code.
It renders a complex GUI in 1-10 milliseconds on a modern laptop.

# Example:
This renders in 7 ms on my MacBook Pro:

![Software rendered](screenshots/imgui_sw.png)

# Future work:
* We do not yet support painting with any other texture than the default font texture.
* Make OpenGL dependency optional
* Optimize rendering of gradient rectangles (common for color pickers)

# How to test it
```
git clone https://github.com/emilk/imgui_software_renderer.git
cd imgui_software_renderer
git submodule update --init --recursive
./build_and_run.sh
```
