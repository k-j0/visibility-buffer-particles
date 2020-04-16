# Main Application - V-Buffer Particles
<!-- This file contains MarkDown formatting. Please open in a MarkDown viewer. -->

This folder contains the main application source code and project files, which implements 4 renderers in C++/GLSL using Vulkan.
## Running the release version
A compiled version of the application for Windows 64 can be found under `Bin/`. Run __vBufferParticles.exe__ to get started.
The application also accepts several command-line arguments, in the format `-key:value`. All key-value pairs must be separated by a single space. For example:
```bash
"vBufferParticles.exe" -ui:0 -renderer:g3 -freeze:1
```
The available keys and values are as follows:
| Key | Value | Default value | Effect |
|---|---|---|---|
| ui | `0` or `1` | `1` | Toggles ImGui (Can also use T key at run-time) |
| freeze | `0` or `1` | `0` | Whether to start with time frozen at t=0s |
| width | any positive integer | `1024` | Initial window resolution width |
| height | any positive integer | `768` | Initial window resolution height |
| shadercomp | `0` or `1` | `0` | Whether to recompile all shaders from source |
| renderer | `v`, `g3`, `g6` or `fwd` | `v` | Initial renderer used; V-Buffer, G-Buffer (3 or 6), Forward |
| pmode | `ve`, `ge`, `co` or `vege` | `ve` | Initial geometry generation mode (vert/vert, geom/geom, comp/comp, vert/geom) |
| pspread | any positive value | `0.4` | Initial particle spread setting |
| psize | any positive value | `0.03` | Initial particle size |
| pcount | any positive integer | `1048576` | Initial particle count |
| pcomplexity | `0`, `1`, `2` or `3` | (saved) | Initial particle complexity level |
| cutout | `0` or `1` | `0` | Whether to start with cut-out particles |

<ins>Note</ins>: Repeated key-values will be ignored, only the last one will be taken into account. Keys not in this table will be ignored. All parameters can be changed within the application at run-time.
### ImGui settings
The ImGui window within the application offers several parameters that can also be changed at run-time. To toggle the window, hit the T key.

The `Controls` header contains a reminder of the available keyboard controls; note that these are also printed to the console upon starting the program.

`Freeze Time` force the time to remain at t=0s to prevent all animations. Note that for some settings to apply, `Freeze Time` must be disabled and can then be re-enabled.

The renderer can be chosen from a drop-down list; the options are `Visibility Buffer`, `Geometry Buffer (3)` (3 framebuffers), `Geometry Buffer (6)` (6 framebuffers) and `Forward Renderer`. Each has a different set of options, but the common ones are highlighted below.

The first dropdown (except in Forward rendering) allows picking which view to render (`Shaded` by default; can also view UVs, Primitive & Material IDs, Depths, Albedo, Emission & Specular colours, World space positions & surface normals, and Metallic coefficients).

The `Particles Only` checkbox toggles whether the rest of the scene is rendered in addition to the particles.

The `Particle Complexity` can be set from 0 (less complex) to 3 (extreme level); additionally cut-out particles can be enabled with particle complexity level 2.

The `GenMode` is the geometry generation mode; the options are `VertexGenExp` for vert/vert mode, `ComputeGenExp` for comp/comp, `GeometryGenExp` for geom/geom, and `VertexGenGeometryExp` for vert/geom.

The particle `Count`, `Half Size`, `Spread`, `Gravity` and `Upwards Force` are also accessible and should be self-explanatory.
## Compiling and running the Debug version
This folder contains all source C++ and GLSL code files, as well as Visual Studio 2019 project settings; the project can be opened by selected __vBufferParticles.sln__. If using another IDE, make sure to enable C++17 and link all dependencies. Some code may need to be adapted for operating systems other than Windows 32 & 64.
### Dependencies
The project uses the following dependencies:
- [Vulkan SDK 1.1.121.2](https://www.lunarg.com/vulkan-sdk/)
- [GLFW 3.3](https://www.glfw.org/download.html)
- [GLM](https://github.com/g-truc/glm)
- [ImGui](https://github.com/ocornut/imgui)
- [STB](https://github.com/nothings/stb)


