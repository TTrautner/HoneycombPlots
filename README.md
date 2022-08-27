# Honeycomb Plots

Repository: https://github.com/TTrautner/HoneycombPlots

### Abstract
Aggregation through binning is a commonly used technique for visualizing large, dense, and overplotted two-dimensional data sets. 
However, aggregation can hide nuanced data-distribution features and complicates the display of multiple data-dependent variables, 
since color mapping is the primary means of encoding. In this paper, we present novel techniques for enhancing hexplots with 
spatialization cues while avoiding common disadvantages of three-dimensional visualizations. In particular, we focus on techniques 
relying on preattentive features that exploit shading and shape cues to emphasize relative value differences. Furthermore, we 
introduce a novel visual encoding that conveys information about the data distributions or trends within individual tiles. 
Based on multiple usage examples from different domains and real-world scenarios, we generate expressive visualizations that 
increase the information content of classic hexplots and validate their effectiveness in a user study.


## Setup on Windows

### Prerequisites

The project uses [CMake](https://cmake.org/) and relies on the following libraries: 

- [GLFW](https://www.glfw.org/) 3.3 or higher (https://github.com/glfw/glfw.git) for windowing and input support
- [glm](https://glm.g-truc.net/) 0.9.9.5 or higher (https://github.com/g-truc/glm.git) for its math funtionality
- [glbinding](https://github.com/cginternals/glbinding) 3.1.0 or higher (https://github.com/cginternals/glbinding.git) for OpenGL API binding
- [globjects](https://github.com/cginternals/globjects) 2.0.0 or higher (https://github.com/cginternals/globjects.git) for additional OpenGL wrapping
- [Dear ImGui](https://github.com/ocornut/imgui) 1.71 or higher (https://github.com/ocornut/imgui.git) for GUI elements
- [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/) 3.3.9 or higher (https://git.code.sf.net/p/tinyfiledialogs/code) for dialog functionality

- Microsoft Visual Studio 2015 or 2017 (2017 is recommended as it offers CMake integration)

### Setup

- Open a shell and run ./fetch-libs.cmd to download all dependencies.
- Run ./build-libs.cmd to build the dependencies.
- Run ./configure.cmd to create the Visual Studio solution files (only necessary for Visual Studion versions prior to 2017).
- Open ./build/molumes.sln in Visual Studio (only necessary for Visual Studion versions prior to 2017, in VS 2017 the CMakeFiles.txt can be opened directly).

### License

Copyright (c) 2022, Thomas Trautner. Released under the [GPLv3 License](LICENSE.md).
Please visite https://vis.uib.no/team/thomas-trautner/ for contact information.
