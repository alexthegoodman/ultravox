# Ultravox

## About

Voxel-based game engine intended for high-performance, beautiful visuals, and open worlds.

## Building

- Install Vulkan SDK (even if you have Vulkan by default)
- `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake`
- `cmake --build build`

## Development

GLSL to SPV

- `glslc shaders/shader.vert -o shaders/shader.vert.spv`
- `glslc shaders/shader.frag -o shaders/shader.frag.spv`
