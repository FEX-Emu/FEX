#pragma once
#include <functional>
#include <memory>

namespace GLContext {
class Context {
public:
  virtual ~Context() {}
  virtual void Create(const char *Title) = 0;
  virtual void Shutdown() = 0;
  virtual void Swap() = 0;
  virtual void GetDim(uint32_t *Dim) = 0;

  using ResizeEvent = std::function<void(uint32_t, uint32_t)>;
  virtual void RegisterResizeEvent(ResizeEvent Event) = 0;
  virtual void* GetWindow() = 0;
};

std::unique_ptr<Context> CreateContext();
}
