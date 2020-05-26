#include "Context.h"

#include <cassert>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdio>

namespace GLContext {
void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

class GLFWContext final : public GLContext::Context {
public:
  void Create(const char *Title) override {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
      assert(0 && "Couldn't init glfw");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_RESIZABLE, 1);
    glfwWindowHint(GLFW_DOUBLEBUFFER, 1);

    Window = glfwCreateWindow(640, 640, Title, nullptr, nullptr);
    if (!Window) {
      assert(0 && "Couldn't create window");
    }
    glfwMakeContextCurrent(Window);
    glfwSwapInterval(1);
  }

  void Shutdown() override {
    glfwMakeContextCurrent(nullptr);
    glfwDestroyWindow(Window);
    glfwTerminate();
  }

  void Swap() override {
    glfwSwapBuffers(Window);
    CheckWindowDimensions();
  }

  void RegisterResizeEvent(ResizeEvent Event) override {
    ResizeEvents.emplace_back(Event);
  }

  void GetDim(uint32_t *Dim) override {
    Dim[0] = Width;
    Dim[1] = Height;
  }

private:

  void CheckWindowDimensions() {
    int LocalWidth;
    int LocalHeight;
    glfwGetWindowSize(Window, &LocalWidth, &LocalHeight);

    if (LocalHeight != Height || LocalWidth != Width) {
      Width = LocalWidth;
      Height = LocalHeight;

      for (auto const &Event : ResizeEvents) {
        Event(Width, Height);
      }
    }
  }
  void* GetWindow() override {
    return Window;
  }

  std::vector<ResizeEvent> ResizeEvents;

  GLFWwindow *Window;
  uint32_t Width{};
  uint32_t Height{};
};

std::unique_ptr<Context> CreateContext() {
  return std::make_unique<GLFWContext>();
}

}
