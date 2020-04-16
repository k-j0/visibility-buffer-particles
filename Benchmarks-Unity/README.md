# Analysis tool - V-Buffer Particles
<!-- This file contains MarkDown formatting. Please open in a MarkDown viewer. -->
This folder contains a Unity project, which can be opened in [Unity 2019.3](https://unity3d.com/get-unity/download). The scripts can be used to perform analysis on data gathered by the [benchmarks tool](../vbparts-benchmarks), or on individual frame renders made by the [main application](../vBufferParticles).
## Analysing performance data
The `ParsePerformanceData.cs` script can be fed collections of CSV files, to be combined into single spreadsheets. These can either be read directly, or parsed further by `PerformanceResults.cs` to produce human-readable CSV files which can be opened in Microsoft Excel or other spreadsheet viewing software.

These CSV files can also be used to generate charts automatically, for example by using a Python script with [Plotly](https://plotly.com/python/). An example of such a script is available [here](./ChartGenerator.py) (also requires [Numpy](https://numpy.org/) and [Pandas](https://pandas.pydata.org/)).
## Analysing visual results
Frames taken from the [main application](../vBufferParticles) can be exported as image files by a tool such as [RenderDoc](https://renderdoc.org/). Examples of such frames are included in the [pixelwise folder](./Assets/pixelwise/textures).

The `TextureComparison.cs` script can take these frames and compare them pixel by pixel to obtain quantitative data in terms of how close or far the results are visually. The Unity HLSL shader `TextureComparisonShader.shader` can also be used to compare textures 2 by 2 and display the absolute difference between them, magnified by any factor to make it visible.
