# Vulkan-Mandelbrot-Set
A Mandelbrot Set renderer made with vulkan and win32 API.
## The Goal
The goal of this project is to create a realtime mandelbrot set renderer with adjustable parameters and navigation, with
option of offline rendering with use of compute shaders to an output PNG file.
### 🛠️ Build 🧰
To build the project, navigate to the build directory and run the setup batch file. There is no need to compile shaders by hand as they are already provided in SPIRV format, but the GLSL source code can be found. Currently, only windows is supported.
### Showcase
![10kIters](https://github.com/CzekoladowyKocur/Vulkan-Mandelbrot-Set/blob/master/showcase/TenThousandIterations.png)
![OfflineRendering](https://github.com/CzekoladowyKocur/Vulkan-Mandelbrot-Set/blob/master/showcase/ComputeMandelbrot.png)
### Input:
#### [W] - Move up
#### [S] - Move down
#### [A] - Move left
#### [D] - Move right
#### [Z] - Zoom In       
#### [X] - Zoom Out
#### [UP] - Increase iterations
#### [DOWN] - Decrease iterations
