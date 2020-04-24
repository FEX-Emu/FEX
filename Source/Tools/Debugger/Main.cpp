#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"
#include "CommonCore/VMFactory.h"
#include "Tests/HarnessHelpers.h"
#include "MainWindow.h"
#include "Context.h"
#include "GLUtils.h"
#include "IMGui_I.h"
#include "DebuggerState.h"
#include "LogManager.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Debug/ContextDebug.h>
#include <FEXCore/Memory/SharedMem.h>
#include <FEXCore/Utils/Event.h>

#include <imgui.h>
#include <epoxy/gl.h>
#include <memory>
#include <GLFW/glfw3.h>
#include <thread>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Event SteppingEvent;
FEXCore::Context::Context *CTX{};
FEXCore::SHM::SHMObject *SHM{};
std::atomic_bool ShouldClose = false;
std::thread CoreThread;

void ExitHandler(uint64_t thread, FEXCore::Context::ExitReason reason) {
    if (ShouldClose)
      FEXCore::Context::Stop(CTX);
    FEX::DebuggerState::SetHasNewState();
    FEX::DebuggerState::CallNewState();
}

void StepCallback() {
  FEXCore::Context::Step(CTX);
}

void CreateCoreCallback(char const *Filename, bool ELF) {
  SHM = FEXCore::SHM::AllocateSHMRegion(1ULL << 32);
  CTX = FEXCore::Context::CreateNewContext();

  FEXCore::Context::AddGuestMemoryRegion(CTX, SHM);

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DEFAULTCORE, FEX::DebuggerState::GetCoreType());
  FEXCore::Context::SetFallbackCPUBackendFactory(CTX, VMFactory::CPUCreationFactoryFallback);

  FEXCore::Context::InitializeContext(CTX);

  bool Result{};
  if (ELF) {
    FEX::HarnessHelper::ELFCodeLoader Loader{Filename, {}, {}, {}};
    Result = FEXCore::Context::InitCore(CTX, &Loader);
  }
  else {
    std::string ConfigName = Filename;
    ConfigName.erase(ConfigName.end() - 3, ConfigName.end());
    ConfigName += "config.bin";
    LogMan::Msg::I("Opening '%s'", Filename);
    LogMan::Msg::I("Opening '%s'", ConfigName.c_str());
    FEX::HarnessHelper::HarnessCodeLoader Loader{Filename, ConfigName.c_str()};
    Result = FEXCore::Context::InitCore(CTX, &Loader);
  }

  LogMan::Throw::A(Result, "Couldn't initialize CTX");

  FEX::DebuggerState::SetContext(CTX);
  FEX::DebuggerState::SetHasNewState();
  FEX::DebuggerState::CallNewState();

  FEXCore::Context::SetExitHandler(CTX, ExitHandler);
}

void CompileRIPCallback(uint64_t RIP) {
  FEXCore::Context::Debug::CompileRIP(CTX, RIP);
  FEX::DebuggerState::SetHasNewState();
  FEX::DebuggerState::CallNewState();
}

void CloseCallback() {
  FEXCore::Context::Stop(CTX);

  if (SHM) {
    FEXCore::SHM::DestroyRegion(SHM);
  }

  if (CTX) {
    FEXCore::Context::DestroyContext(CTX);
  }

  SHM = nullptr;
  CTX = nullptr;

  ShouldClose = false;
  FEX::DebuggerState::SetContext(CTX);
  FEX::DebuggerState::SetHasNewState();
  FEX::DebuggerState::CallNewState();
}

void PauseCallback() {
  FEXCore::Context::Pause(CTX);
  FEX::DebuggerState::SetHasNewState();
  FEX::DebuggerState::CallNewState();
}

void ContinueCallback() {
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SINGLESTEP, FEX::DebuggerState::GetRunningMode());
  SteppingEvent.NotifyAll();
}

void GetIRCallback(std::stringstream *out, uint64_t PC) {
  if (!CTX) {
    *out << "<No Core>";
    return;
  }
  // FEXCore::IR::IntrusiveIRList *ir;
  // bool HadIR = FEXCore::Context::Debug::FindIRForRIP(FEX::DebuggerState::GetContext(), PC, &ir);
  // if (HadIR) {
  //   FEXCore::IR::Dump(out, ir);
  // }
  // else {
  //   *out << "<No IR Found>";
  // }
}

int main(int argc, char **argv, char **const envp) {
  FEX::Config::Init();
  FEX::EnvLoader::Load(envp);
  FEX::ArgLoader::Load(argc, argv);

  FEXCore::Context::InitializeStaticTables();

  auto Context = GLContext::CreateContext();
  Context->Create("FEX Debugger");
  FEX::Debugger::Init();
  FEX::DebuggerState::RegisterStepCallback(StepCallback);
  FEX::DebuggerState::RegisterPauseCallback(PauseCallback);
  FEX::DebuggerState::RegisterContinueCallback(ContinueCallback);
  FEX::DebuggerState::RegisterCreateCallback(CreateCoreCallback);
  FEX::DebuggerState::RegisterCompileRIPCallback(CompileRIPCallback);

  FEX::DebuggerState::RegisterCloseCallback(CloseCallback);
  FEX::DebuggerState::RegisterGetIRCallback(GetIRCallback);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

  GLFWwindow *Window = static_cast<GLFWwindow*>(Context->GetWindow());
  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(Window, true);
  const char* glsl_version = "#version 410";
  ImGui_ImplOpenGL3_Init(glsl_version);

  while (!glfwWindowShouldClose(Window)) {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    FEX::Debugger::DrawDebugUI(Context.get());

    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    Context->Swap();
  }

  FEX::Debugger::Shutdown();

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  Context->Shutdown();

  FEX::Config::Shutdown();

  return 0;
}
