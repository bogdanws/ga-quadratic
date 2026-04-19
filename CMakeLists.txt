cmake_minimum_required(VERSION 3.21)
project(ga_lab CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GA_BUILD_GUI "Build the ImGui+ImPlot GUI" OFF)

add_library(ga_core
    src/ga/rng.cpp
    src/ga/problem.cpp
    src/ga/chromosome.cpp
    src/ga/population.cpp
    src/ga/ga.cpp
    src/io/config.cpp
    src/io/logger.cpp)
target_include_directories(ga_core PUBLIC src)
target_compile_options(ga_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(ga src/main.cpp)
target_compile_options(ga PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(ga PRIVATE ga_core)

if(GA_BUILD_GUI)
    include(FetchContent)

    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)

    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.92.7
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(imgui)

    FetchContent_Declare(implot
        GIT_REPOSITORY https://github.com/epezent/implot.git
        GIT_TAG        v1.0
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(implot)

    add_library(imgui_static STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${implot_SOURCE_DIR}/implot.cpp
        ${implot_SOURCE_DIR}/implot_items.cpp)
    target_include_directories(imgui_static PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${implot_SOURCE_DIR})
    target_link_libraries(imgui_static PUBLIC glfw)
    # silence warnings from third-party headers we don't control
    target_compile_options(imgui_static PRIVATE -w)

    find_package(OpenGL REQUIRED)

    target_sources(ga PRIVATE
        src/gui/platform.cpp
        src/gui/app.cpp)
    target_link_libraries(ga PRIVATE imgui_static OpenGL::GL)
    target_compile_definitions(ga PRIVATE GA_BUILD_GUI)
endif()
