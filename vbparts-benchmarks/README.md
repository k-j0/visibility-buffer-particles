# Benchmarking Tool - V-Buffer Particles
<!-- This file contains MarkDown formatting. Please open in a MarkDown viewer. -->

This tool can be used to automatically benchmark the [main application](../vBufferParticles) in a number of different scenarios.
## Compiling the code
This application is made using C++/CLI, so is reserved for Windows usage only. The Win64 binaries are provided in the [Bin](./Bin) folder, but the Visual Studio 2019 project files are also included to compile from source.
## Running Benchmarks
The benchmarking tool depends on [MSI Afterburner](https://www.msi.com/page/afterburner), which should be installed separately, to properly function. MSI Afterburner should be set to log performance data to the [Benchmarks](./Benchmarks) folder, with the exact file name `__.hml`. The exact data collected by MSI Afterburner can be setup in the parameters; at least frametime and memory usage are recommended to obtain operable results.

From a console instance with admin privileges, `cd` into the [vBufferParticles](../vBufferParticles) folder, ensure MSI Afterburner is opened in graph view, and start the tool using the following command:
```bash
"../vbparts-benchmarks/Bin/vbparts-benchmarks"
```

The following command line arguments can be added to modify the behaviour of the program:

|argument|effect|
| --- | --- |
|`-no-usual`|Usual tests will be bypassed; use in combination with other arguments|
|`-full-count`|Will run 320 additional particle count tests|
|`-full-size`|Will run 164 additional particle size tests|
|`-cutout`|Will use cut-out particles for all tests; note that this may produce unexpected results when using particle complexities != 2|
|`@`___n___|Override the test length, in seconds, to ___n___ seconds (must be at least 6 seconds)|

Prompts will appear to place your cursor in a few designated locations on screen within the MSI Afterburner window, to automatically log the results as the main application is launched upwards of 270 times (in the default settings). Then, the full benchmark should last around 3 hours (again depending on the settings used).

Results will be stored into the [Benchmarks](./Benchmarks) folder as individual CSV files. These can be fed directly into the [C# Unity benchmarking tool](../Benchmarks-Unity).

The CSV files will be named `r_m_c_wxh_l_d_s.csv` with the following values:
|Shortcut|Represents|Possible values|
|---|---|---|
|`r`|Renderer|`v`, `g3`, `g6` or `fwd`|
|`m`|Geometry generation mode|`ve`, `ge`, `co` or `vege`|
|`c`|Particle count|positive integer|
|`w`|Window width|positive integer|
|`h`|Window height|positive integer|
|`l`|Complexity level|`0`, `1`, `2` or `3`|
|`d`|Density|positive integer; to be divided by 1000|
|`s`|Size|positive integer; to be divided by 1000|

