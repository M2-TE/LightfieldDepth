# LightfieldDepth
Deriving depth from lightfield data. Built using Vulkan.

# Compile:
## Windows
1. Open the VulkanEngine.sln file using Visual Studio 2022
2. Select Vermillion-Windows as the startup project
3. Select build configuration (Debug/Release)
4. Build and pray

## Linux
Note: Building for Linux requires Visual Studio 2022 as well, so either use a Windows machine in the same local network or in a VM in addition to your Linux machine.
### Steps for Linux (Debian-based):
1. SDL2: sudo apt install libsdl2-dev
2. Vulkan SDK: (Ubuntu) -> https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html

### Steps for Windows:
1. Open the VulkanEngine.sln file using Visual Studio 2022
2. Select build configuration (Debug-Linux/Release-Linux)
3. Build and follow the prompts to set up an SSH connection to a Linux machine (will build the project on said machine remotely)
