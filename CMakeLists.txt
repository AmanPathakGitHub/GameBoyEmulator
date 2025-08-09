
cmake_minimum_required(VERSION 3.24.2)
set(CMAKE_CXX_STANDARD 20)

project(GameBoyEmulator)

include(FetchContent)

enable_testing()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# Prevent GoogleTest from overriding our compiler/linker options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


find_package(raylib)
if (NOT raylib_FOUND)
    
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    )
    FetchContent_MakeAvailable(raylib)
endif()


FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9-docking
)

FetchContent_MakeAvailable(imgui)

# Manually add necessary files
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG 0105acf1bfba22e883688e5b9ffb1af0536739e2
)

FetchContent_MakeAvailable(rlImGui)

add_library(rlImGui STATIC
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)

target_link_libraries(rlImGui PUBLIC raylib imgui)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui)


file(GLOB SOURCE_FILES "src/*.cpp")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*main\\.cpp$")
file(GLOB INCLUDE_FILES "include/*.h")

add_library(GameBoyLib ${SOURCE_FILES} ${INCLUDE_FILES})
target_include_directories(GameBoyLib PUBLIC "include/" ${raylib_SOURCE_DIR}/src ${rlimgui_SOURCE_DIR})
target_link_libraries(GameBoyLib glfw raylib rlImGui imgui)

target_compile_definitions(GameBoyLib
    PRIVATE
        # If the debug configuration pass the DEBUG define to the compiler 
        $<$<CONFIG:Debug>:DEBUG>
)

add_executable(GameBoyTests "tests/main.cpp")
target_link_libraries(GameBoyTests GameBoyLib gtest_main)

add_test(NAME CPU_TESTS COMMAND GameBoyTests)

add_executable(GameBoyEmulator "src/main.cpp")
target_link_libraries(GameBoyEmulator GameBoyLib)