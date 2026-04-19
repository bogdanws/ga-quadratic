#include "gui/platform.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <iostream>
#include <stdexcept>

namespace gui {

static void glfwErrorCallback(int code, const char* desc) {
    std::cerr << "GLFW error " << code << ": " << desc << "\n";
}

Platform::Platform(const char* title, int w, int h) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        throw std::runtime_error("GLFW init failed");

    // request OpenGL 3.3 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("GLFW window creation failed");
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

Platform::~Platform() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window_);
    glfwTerminate();
}

bool Platform::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void Platform::beginFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Platform::endFrame() {
    ImGui::Render();

    int fw, fh;
    glfwGetFramebufferSize(window_, &fw, &fh);
    glViewport(0, 0, fw, fh);
    glClearColor(0.067f, 0.067f, 0.106f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);
}

} // namespace gui
