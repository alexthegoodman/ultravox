cmake_minimum_required(VERSION 3.21)
project(Ultravox VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Fetch latest ImGuizmo from GitHub
FetchContent_Declare(
    ImGuizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG master   # or a specific commit/tag if you want stability
)

FetchContent_MakeAvailable(ImGuizmo)

# Find vcpkg packages
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
# find_package(imguizmo CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)

# Define your executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/PhysicsSystem.cpp
    src/PlayerCharacter.cpp
    src/Sphere.cpp
    src/TerrainGenerator.cpp
    src/Inventory.cpp
    src/components/Tree.cpp
    src/components/House.cpp
    src/components/WarTornDome.cpp
    src/items/Item.cpp
    src/items/Weapon.cpp
    src/items/Apple.cpp
    src/items/Laser.cpp
    src/items/WorldItem.cpp
    # src/Camera3D.cpp
    # src/ChunkManager.cpp
    # other files currently included in main.cpp
    # Add ImGuizmo source files directly to your target
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    ${imguizmo_SOURCE_DIR}/ImGuizmo.h
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glm::glm
    glfw
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    imgui::imgui
    # imguizmo::imguizmo
    # ImGuizmo   # <-- note: capital I, matches the CMake target defined by the repo
    Jolt::Jolt
)

# Include directories (if needed beyond what the targets provide)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${imguizmo_SOURCE_DIR}
)

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Copy assets or shaders to build directory if needed
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Would hate to have stale shaders in build dir!
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SHADER_FILES ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag)

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_DIR} ${SHADER_OUTPUT_DIR}
    COMMENT "Copying shaders..."
)

# Make your executable depend on them
add_dependencies(${PROJECT_NAME} copy_shaders)
