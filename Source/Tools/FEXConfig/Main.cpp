#include "Common/Config.h"
#include "Context.h"

#include <imgui.h>
#define YES_IMGUIFILESYSTEM
#include <addons/imgui_user.h>
#include <epoxy/gl.h>
#include <memory>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <thread>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace {
  static double GlobalTime{};

  static bool ConfigOpen{};
  static bool ConfigChanged{};
  static int EnvironmentVariableSelected{};

  static std::string ConfigFilename{};
  static std::unique_ptr<FEXCore::Config::Layer> LoadedConfig{};

  static const char EnvironmentPopupName[] = "#New Environment Variable";
  static const char SavedPopupAppName[] = "#SavedApp";

  static bool OpenMsgPopup{};
  static std::string MsgMessage{};
  static const char MsgPopupName[] = "#Msg";
  static std::chrono::high_resolution_clock::time_point MsgTimerStart{};

  static bool SelectedOpenFile{};
  static bool SelectedSaveFileAs{};
  static ImGuiFs::Dialog DialogSaveAs{};
  static ImGuiFs::Dialog DialogOpen{};

  void OpenMsgMessagePopup(std::string Message) {
    OpenMsgPopup = true;
    MsgMessage = Message;
    MsgTimerStart = std::chrono::high_resolution_clock::now();
  }

  void OpenFile(std::string Filename) {
    if (!std::filesystem::exists(Filename)) {
      OpenMsgMessagePopup("Couldn't open: " + Filename);
      return;
    }
    ConfigOpen = true;
    ConfigFilename = Filename;
    LoadedConfig = std::make_unique<FEX::Config::MainLoader>(Filename);
    LoadedConfig->Load();
  }

  void LoadDefaultSettings() {
    ConfigOpen = true;
    ConfigFilename = {};
    LoadedConfig = std::make_unique<FEX::Config::EmptyMapper>();
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE,        "1");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_MAXBLOCKINST,       "5000");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_SINGLESTEP,         "0");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK,         "1");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_GDBSERVER,          "0");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_EMULATED_CPU_CORES, "1");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ROOTFSPATH,         "");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_THUNKLIBSPATH,      "");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_SILENTLOGS,         "0");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT,        "GALLIUM_THREAD=1");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT,        "GALLIUM_HUD=fps");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT,        "TERM=xterm");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_OUTPUTLOG,          "");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_DUMPIR,             "no"),
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_TSO_ENABLED,        "1");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_SMC_CHECKS,         "0");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ABI_LOCAL_FLAGS,    "0");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ABI_NO_PF,          "0");
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_DEBUG_DISABLE_OPTIMIZATION_PASSES, "0");
  }

  void SaveFile(std::string Filename) {
    if (!ConfigOpen) {
      OpenMsgMessagePopup("Can't save file when config isn't open");
      return;
    }

    FEX::Config::SaveLayerToJSON(Filename, LoadedConfig.get());
    ConfigChanged = false;
    ConfigFilename = Filename;
    OpenMsgMessagePopup("Config Saved to: '" + Filename + "'");

    // Output in terminal as well
    printf("Config Saved to: '%s'\n", ConfigFilename.c_str());
  }

  void CloseConfig() {
    ConfigOpen = false;
    ConfigFilename = {};
    ConfigChanged = false;
    LoadedConfig.reset();
  }

  void FillCPUConfig() {
    char BlockSize[32]{};
    char EmulatedCPUCores[32]{};

    if (ImGui::BeginTabItem("CPU")) {
      ImGui::Text("Core:");
      auto Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE);

      ImGui::SameLine();
      if (ImGui::RadioButton("Int", Value.has_value() && **Value == "0")) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE, "0");
        ConfigChanged = true;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("JIT", Value.has_value() && **Value == "1")) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE, "1");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_MAXBLOCKINST);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(BlockSize, &(*Value)->at(0), 32);
      }
      if (ImGui::InputText("Block Size:", BlockSize, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_MAXBLOCKINST, BlockSize);
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK);
      bool Multiblock = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Multiblock", &Multiblock)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK, Multiblock ? "1" : "0");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_EMULATED_CPU_CORES);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(EmulatedCPUCores, &(*Value)->at(0), 32);
      }
      if (ImGui::InputText("Emulated CPU cores:", EmulatedCPUCores, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_EMULATED_CPU_CORES, EmulatedCPUCores);
        ConfigChanged = true;
      }

      ImGui::EndTabItem();
    }
  }

  bool EnvironmentVariableFiller(void *data, int idx, const char** out_text) {
    static char TmpString[256];
    auto Value = LoadedConfig->All(FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT);
    if (Value.has_value()) {
      auto List = (*Value);
      auto it = List->begin();

      // Since this is a list, we don't have a linear allocator that we can just jump to an element
      // Just do a quick spin
      for (int i = 0; i < idx; ++i)
        ++it;

      snprintf(TmpString, 256, "%s", it->c_str());
      *out_text = TmpString;

      return true;
    }

    return false;
  }

  void DeleteEnvironmentVariable(int idx) {
    auto Value = LoadedConfig->All(FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT);
    auto List = (*Value);
    auto it = List->begin();

    // Since this is a list, we don't have a linear allocator that we can just jump to an element
    // Just do a quick spin
    for (int i = 0; i < idx; ++i)
      ++it;

    List->erase(it);
    ConfigChanged = true;
  }

  void AddNewEnvironmentVariable() {
    char Environment[256]{};

    if (ImGui::BeginPopup(EnvironmentPopupName)) {
      if (ImGui::InputText("New Environment", Environment, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT, Environment);
        ImGui::CloseCurrentPopup();
        ConfigChanged = true;
      }

      ImGui::EndPopup();
    }
  }

  void FillEmulationConfig() {
    char RootFS[256]{};
    char ThunkPath[256]{};

    int NumEnvironmentVariables{};

    if (ImGui::BeginTabItem("Emulation")) {
      auto Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_ROOTFSPATH);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(RootFS, &(*Value)->at(0), 256);
      }
      if (ImGui::InputText("RootFS:", RootFS, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_ROOTFSPATH, RootFS);
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_THUNKLIBSPATH);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(ThunkPath, &(*Value)->at(0), 256);
      }
      if (ImGui::InputText("Thunk library folder:", ThunkPath, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_THUNKLIBSPATH, ThunkPath);
        ConfigChanged = true;
      }


      auto ValueList = LoadedConfig->All(FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT);
      if (ValueList.has_value()) {
        NumEnvironmentVariables = (*ValueList)->size();
      }

      ImGui::Text("Number of environment variables: %d", NumEnvironmentVariables);

      ImGui::ListBox("Environment variables", &EnvironmentVariableSelected, EnvironmentVariableFiller, nullptr, NumEnvironmentVariables);

      if (ImGui::SmallButton("+")) {
        ImGui::OpenPopup(EnvironmentPopupName);
      }

      // Only draws if popup is open
      AddNewEnvironmentVariable();

      if (NumEnvironmentVariables) {
        ImGui::SameLine();
        if (ImGui::SmallButton("-")) {
          DeleteEnvironmentVariable(EnvironmentVariableSelected);
          EnvironmentVariableSelected = std::max(0, EnvironmentVariableSelected - 1);
        }
      }

      ImGui::Text("Debugging:");
      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_DEBUG_DISABLE_OPTIMIZATION_PASSES);
      bool DisablePasses = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Disable Optimization Passes", &DisablePasses)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_DEBUG_DISABLE_OPTIMIZATION_PASSES, DisablePasses ? "1" : "0");
        ConfigChanged = true;
      }

      ImGui::EndTabItem();
    }
  }

  void FillLoggingConfig() {
    char LogFile[256]{};
    char IRDump[256]{};

    if (ImGui::BeginTabItem("Logging")) {
      auto Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_SILENTLOGS);
      bool SilentLog = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Silent Logging", &SilentLog)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_SILENTLOGS, SilentLog ? "1" : "0");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_OUTPUTLOG);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(LogFile, &(*Value)->at(0), 256);
      }
      if (ImGui::InputText("Output log file:", LogFile, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_OUTPUTLOG, LogFile);
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_DUMPIR);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(IRDump, &(*Value)->at(0), 256);
      }
      if (ImGui::InputText("IR Dump location:", IRDump, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_DUMPIR, IRDump);
        ConfigChanged = true;
      }

      ImGui::EndTabItem();
    }
  }

  void FillHackConfig() {
    if (ImGui::BeginTabItem("Hacks")) {
      auto Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_TSO_ENABLED);
      bool TSOEnabled = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("TSO Enabled", &TSOEnabled)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_TSO_ENABLED, TSOEnabled ? "1" : "0");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_SMC_CHECKS);
      bool SMCChecks = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("SMC Checks", &SMCChecks)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_SMC_CHECKS, SMCChecks ? "1" : "0");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_ABI_LOCAL_FLAGS);
      bool UnsafeLocalFlags = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Unsafe local flags optimization", &UnsafeLocalFlags)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_ABI_LOCAL_FLAGS, UnsafeLocalFlags ? "1" : "0");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_ABI_NO_PF);
      bool NoPFCalculation = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Disable PF calculation", &NoPFCalculation)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_ABI_NO_PF, NoPFCalculation ? "1" : "0");
        ConfigChanged = true;
      }

      ImGui::EndTabItem();
    }
  }

  void FillConfigWindow() {
    ImGui::BeginTabBar("Config");
    FillCPUConfig();
    FillEmulationConfig();
    FillLoggingConfig();
    FillHackConfig();
    ImGui::EndTabBar();
  }

  void DrawUI(GLContext::Context *Context) {
    ImGuiIO& io = ImGui::GetIO();
    double current_time = glfwGetTime();
    io.DeltaTime = GlobalTime > 0.0 ? static_cast<float>(current_time - GlobalTime) : static_cast<float>(1.0f/60.0f);
    GlobalTime = current_time;

    ImGui::NewFrame();

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGui::DockSpace(ImGui::GetID("DockSpace"));

    struct {
      bool Open{};
      bool OpenDefault{};
      bool LoadDefault{};
      bool Save{};
      bool SaveAs{};
      bool SaveDefault{};
      bool Close{};
    } Selected;

    char AppName[256]{};

    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Open", "CTRL+O", &Selected.Open, true);
        ImGui::MenuItem("Open Default", "CTRL+SHIFT+O", &Selected.OpenDefault, true);
        ImGui::MenuItem("Load Default Options", "CTRL+SHIFT+D", &Selected.LoadDefault, true);

        ImGui::MenuItem("Save", "CTRL+S", &Selected.Save, true);
        ImGui::MenuItem("Save As", "CTRL+SHIFT+S", &Selected.SaveAs, true);
        ImGui::MenuItem("Save As App profile", "CTRL+E", nullptr, true);
        ImGui::MenuItem("Save Default", "CTRL+SHIFT+P", &Selected.SaveDefault, true);

        ImGui::MenuItem("Close", "CTRL+W", &Selected.Close, true);

        ImGui::EndMenu();
      }

      ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight());

      if (ConfigOpen) {
        if (ConfigChanged) {
          ImGui::PushStyleColor(ImGuiCol_FrameBg,   ImVec4(1.0, 1.0, 0.0, 1.0));
          ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0, 1.0, 0.0, 1.0));
        }
        else {
          ImGui::PushStyleColor(ImGuiCol_FrameBg,   ImVec4(0.0, 1.0, 0.0, 1.0));
          ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0, 1.0, 0.0, 1.0));
        }
      }
      else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg,   ImVec4(1.0, 0.0, 0.0, 1.0));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0, 0.0, 0.0, 1.0));
      }

      ImGui::RadioButton("", true);
      ImGui::PopStyleColor(2);

      ImGui::EndMenuBar();
    }

    if (ConfigOpen) {
      if (ImGui::Begin("#Config")) {
        FillConfigWindow();
      }

      if (ImGui::IsKeyPressed(GLFW_KEY_E) && io.KeyCtrl) {
        ImGui::OpenPopup(SavedPopupAppName);
      }

      ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x / 2, viewport->Pos.y + viewport->Size.y / 2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
      if (ImGui::BeginPopupModal(SavedPopupAppName)) {
        if (ImGui::InputText("App name", AppName, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
          std::string AppNameString = AppName;
          std::string Filename = FEX::Config::GetApplicationConfig(AppNameString, false);
          SaveFile(Filename);
          ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE)) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      // Need this frame delay loop since ImGui doesn't allow us to enable a popup near the end of the frame
      if (OpenMsgPopup) {
        ImGui::OpenPopup(MsgPopupName);
        OpenMsgPopup = false;
      }

      // Center the saved popup in the center of the window
      ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x / 2, viewport->Pos.y + viewport->Size.y / 2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

      if (ImGui::BeginPopup(MsgPopupName)) {
        ImGui::Text("%s", MsgMessage.c_str());
        if ((std::chrono::high_resolution_clock::now() - MsgTimerStart) >= std::chrono::seconds(2)) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      ImGui::End();
    }

    if (Selected.Open ||
        (ImGui::IsKeyPressed(GLFW_KEY_O) && io.KeyCtrl && !io.KeyShift)) {
      SelectedOpenFile = true;
    }
    if (Selected.OpenDefault ||
        (ImGui::IsKeyPressed(GLFW_KEY_O) && io.KeyCtrl && io.KeyShift)) {
      OpenFile(FEX::Config::GetConfigFileLocation());
    }
    if (Selected.LoadDefault ||
        (ImGui::IsKeyPressed(GLFW_KEY_D) && io.KeyCtrl && io.KeyShift)) {
      LoadDefaultSettings();
    }

    if (Selected.Save ||
        (ImGui::IsKeyPressed(GLFW_KEY_S) && io.KeyCtrl && !io.KeyShift)) {
      SaveFile(ConfigFilename);
    }
    if (Selected.SaveAs ||
        (ImGui::IsKeyPressed(GLFW_KEY_S) && io.KeyCtrl && io.KeyShift)) {
      SelectedSaveFileAs = true;
    }

    if (Selected.SaveDefault ||
        (ImGui::IsKeyPressed(GLFW_KEY_P) && io.KeyCtrl && io.KeyShift)) {
      SaveFile(FEX::Config::GetConfigFileLocation());
    }
    if (Selected.Close ||
        (ImGui::IsKeyPressed(GLFW_KEY_W) && io.KeyCtrl && !io.KeyShift)) {
      CloseConfig();
    }

    ImGui::End(); // End dockspace

    char const *InitialPath;
    char const *File;

    InitialPath = DialogOpen.chooseFileDialog(SelectedOpenFile, "./", ".json", "#Chose a config to load");
    File = DialogOpen.getChosenPath();
    if (strlen(InitialPath) > 0 && strlen(File) > 0) {
      OpenFile(File);
    }

    InitialPath = DialogSaveAs.saveFileDialog(SelectedSaveFileAs, "./", "Config.json", ".json", "#Choose where to save a config");
    File = DialogSaveAs.getChosenPath();
    if (strlen(InitialPath) > 0 && strlen(File) > 0) {
      SaveFile(File);
    }

    SelectedOpenFile = false;
    SelectedSaveFileAs = false;

    ImGui::Render();
  }
}

int main() {
  std::string ImGUIConfig = FEX::Config::GetConfigFolder(false) + "FEXConfig_imgui.ini";

  auto Context = GLContext::CreateContext();
  Context->Create("#FEXConfig");

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
  io.IniFilename = &ImGUIConfig.at(0);

  GLFWwindow *Window = static_cast<GLFWwindow*>(Context->GetWindow());
  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(Window, true);
  const char* glsl_version = "#version 130";
  ImGui_ImplOpenGL3_Init(glsl_version);

  while (!glfwWindowShouldClose(Window)) {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    DrawUI(Context.get());

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

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  Context->Shutdown();

  return 0;
}
