# Visibility Buffer Particles
<!--This file contains MarkDown formatting. Please open in a MarkDown viewer. -->

Particles for use in a Visibility Buffer deferred renderer. The project consists in a main application, and 2 companion applications used to gather performance and visual test results. Please refer to the README file in each individual folder for more details about how to compile and run each application.

## Main Application
The main C++ & GLSL Vulkan graphics application can be found under [vBufferParticles](./vBufferParticles) (both source code and Windows 64 binaries).
## Benchmarks Application
The [vbparts-benchmarks](./vbparts-benchmarks) folder contains a small C++/CLI application that runs a number of tests on the main application.
## Benchmarks Results
The files generated by the [vbparts-benchmarks](./vbparts-benchmarks) application can either be read directly, or fed into the set of C# and HLSL results scripts available under [Benchmarks-Unity](./Benchmarks-Unity), which are meant to be opened within the Unity game engine.