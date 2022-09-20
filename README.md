# Honeycomb Plots

Repository: https://github.com/TTrautner/HoneycombPlots

## Abstract
Aggregation through binning is a commonly used technique for visualizing large, dense, and overplotted two-dimensional data sets. 
However, aggregation can hide nuanced data-distribution features and complicates the display of multiple data-dependent variables, 
since color mapping is the primary means of encoding. In this paper, we present novel techniques for enhancing hexplots with 
spatialization cues while avoiding common disadvantages of three-dimensional visualizations. In particular, we focus on techniques 
relying on preattentive features that exploit shading and shape cues to emphasize relative value differences. Furthermore, we 
introduce a novel visual encoding that conveys information about the data distributions or trends within individual tiles. 
Based on multiple usage examples from different domains and real-world scenarios, we generate expressive visualizations that 
increase the information content of classic hexplots and validate their effectiveness in a user study.


## Prerequisites

The project uses [CMake](https://cmake.org/) and relies on the following libraries: 

- [GLFW](https://www.glfw.org/) 3.3 or higher (https://github.com/glfw/glfw.git) for windowing and input support
- [glm](https://glm.g-truc.net/) 0.9.9.5 or higher (https://github.com/g-truc/glm.git) for its math funtionality
- [glbinding](https://github.com/cginternals/glbinding) 3.1.0 or higher (https://github.com/cginternals/glbinding.git) for OpenGL API binding
- [globjects](https://github.com/cginternals/globjects) 2.0.0 or higher (https://github.com/cginternals/globjects.git) for additional OpenGL wrapping
- [Dear ImGui](https://github.com/ocornut/imgui) 1.71 or higher (https://github.com/ocornut/imgui.git) for GUI elements
- [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/) 3.3.9 or higher (https://git.code.sf.net/p/tinyfiledialogs/code) for dialog functionality
- [stb](https://github.com/nothings/stb/) (stb_image.h 2.22 or higher and stb_image_write.h 1.13 or higher) or higher (https://github.com/nothings/stb.git) for image loading and saving

The project uses vcpkg (https://vcpkg.io) for dependency management, so this should take care of everything. Please follow the instructions on the vcpkg website (https://vcpkg.io/en/getting-started.html) to set it up for your environment. When using visual studio, make sure to install the vcpkg integration using

```
vcpkg integrate install
```

There is a manifest file called ```vcpkg.json``` in the project root folder. When building with CMake for the first time, all dependencies should be downloaded and installed automatically.

## Building

If you are using Visual Studio, you can use its integrated CMake support to build and run the project.

When instead building from the command line, run the following commands from the project root folder:

```
mkdir build
cd build
cmake ..
```

After this, you can compile the debug or release versions of the project using 

```
cmake --build --config Debug
```

and/or

```
cmake --build --config Release
```

After building, the executables will be available in the ```./bin``` folder.

## Running

To correctly locate shaders and other resources (which are stored in the  ```./res``` folder), the program requires the current working directory to be the project root folder. A launch configuration file for Visual Studio (```./.vs/launch.vs.json```) that takes care of this is included, just select ```honeycomb``` as a startup item in the toolbar. When running from the command line, make sure that you are in the project root folder and execute

```
./bin/honeycomb
```
## Usage

After starting the program, choose one of the provided example data sets as "File" from the dropdown menu. The resulting visualization is then generated using default parameters, which can further be customized using the GUI.

## License

Copyright (c) 2022, Thomas Trautner. Released under the [GPLv3 License](LICENSE.md).
Please visite https://vis.uib.no/team/thomas-trautner/ for contact information.
