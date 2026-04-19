#pragma once

// forward declaration so we don't drag GLFW headers into every translation unit
struct GLFWwindow;

namespace gui {

// RAII wrapper around the GLFW window + ImGui + ImPlot
class Platform {
  public:
    Platform(const char* title, int w, int h);
    ~Platform();

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    bool shouldClose() const;
    void beginFrame(); // poll events + start ImGui/ImPlot frame
    void endFrame();   // render + swap buffers

  private:
    GLFWwindow* window_;
};

} // namespace gui
