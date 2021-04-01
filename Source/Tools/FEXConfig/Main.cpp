#include "Common/Config.h"

#include <FEXCore/Utils/Event.h>

#include <imgui.h>
#define YES_IMGUIFILESYSTEM
#include <addons/imgui_user.h>
#include <epoxy/gl.h>
#include <SDL.h>
#include <SDL_scancode.h>
#include <memory>
#include <mutex>
#include <filesystem>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

namespace {
  static std::chrono::time_point<std::chrono::high_resolution_clock> GlobalTime{};

  static bool ConfigOpen{};
  static bool ConfigChanged{};
  static int EnvironmentVariableSelected{};
  static int NamedRootFSSelected{-1};

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

  // Named rootfs
  static std::vector<std::string> NamedRootFS{};
  static std::mutex NamedRootFSUpdator{};

  static std::atomic<int> INotifyFD{-1};
  static int INotifyFolderFD{};
  static std::thread INotifyThreadHandle{};
  static std::atomic_bool INotifyShutdown{};

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

  void LoadNamedRootFSFolder() {
    std::scoped_lock<std::mutex> lk{NamedRootFSUpdator};
    NamedRootFS.clear();
    std::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
    for (auto &it : std::filesystem::directory_iterator(RootFS)) {
      if (it.is_directory()) {
        NamedRootFS.emplace_back(it.path().filename());
      }
    }
  }

  void INotifyThread() {
    while (!INotifyShutdown) {
      constexpr size_t DATA_SIZE = (16 * (sizeof(struct inotify_event) + NAME_MAX + 1));
      char buf[DATA_SIZE];
      struct timeval tv{};
      // 50 ms
      tv.tv_usec = 50000;

      int Ret{};
      do {
        fd_set Set{};
        FD_ZERO(&Set);
        FD_SET(INotifyFD, &Set);
        Ret = select(INotifyFD + 1, &Set, nullptr, nullptr, &tv);
      } while (Ret == 0 && INotifyFD != -1);

      if (Ret == -1 || INotifyFD == -1) {
        // Just return on error
        INotifyShutdown = true;
        return;
      }

      // Spin through the events, we don't actually care what they are
      while (read(INotifyFD, buf, DATA_SIZE) > 0);

      // Now update the named vector
      LoadNamedRootFSFolder();

      // Update the window
      SDL_Event Event{};
      Event.type = SDL_USEREVENT;
      SDL_PushEvent(&Event);
    }
  }

  void SetupINotify() {
    INotifyFD = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    INotifyShutdown = false;

    std::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
    INotifyFolderFD = inotify_add_watch(INotifyFD, RootFS.c_str(), IN_CREATE | IN_DELETE);
    if (INotifyFolderFD != -1) {
      INotifyThreadHandle = std::thread(INotifyThread);
    }
    else {
      printf("Failed INotify thread\n");
    }
  }

  void ShutdownINotify() {
    close(INotifyFD);
    INotifyFD = -1;
    if (INotifyThreadHandle.joinable()) {
      INotifyThreadHandle.join();
    }
  }

  void LoadDefaultSettings() {
    ConfigOpen = true;
    ConfigFilename = {};
    LoadedConfig = std::make_unique<FEX::Config::EmptyMapper>();
#define OPT_BASE(type, group, enum, json, default) \
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(default));
#define OPT_STR(group, enum, json, default) \
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, default);
#define OPT_STRARRAY(group, enum, json, default)  // Do nothing
#include <FEXCore/Config/ConfigValues.inl>
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
      auto Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_CORE);

      ImGui::SameLine();
      if (ImGui::RadioButton("Int", Value.has_value() && **Value == "0")) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_CORE, "0");
        ConfigChanged = true;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("JIT", Value.has_value() && **Value == "1")) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_CORE, "1");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_MAXINST);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(BlockSize, &(*Value)->at(0), 32);
      }
      if (ImGui::InputText("Block Size:", BlockSize, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_MAXINST, BlockSize);
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK);
      bool Multiblock = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Multiblock", &Multiblock)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK, Multiblock ? "1" : "0");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_THREADS);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(EmulatedCPUCores, &(*Value)->at(0), 32);
      }
      if (ImGui::InputText("Emulated CPU cores:", EmulatedCPUCores, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_THREADS, EmulatedCPUCores);
        ConfigChanged = true;
      }

      ImGui::EndTabItem();
    }
  }

  bool EnvironmentVariableFiller(void *data, int idx, const char** out_text) {
    static char TmpString[256];
    auto Value = LoadedConfig->All(FEXCore::Config::ConfigOption::CONFIG_ENV);
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

  bool NamedRootFSVariableFiller(void *data, int idx, const char** out_text) {
    std::scoped_lock<std::mutex> lk{NamedRootFSUpdator};
    static char TmpString[256];
    if (idx >= 0 && idx < NamedRootFS.size()) {
      // Since this is a list, we don't have a linear allocator that we can just jump to an element
      // Just do a quick spin
      snprintf(TmpString, 256, "%s", NamedRootFS.at(idx).c_str());
      *out_text = TmpString;

      return true;
    }

    return false;
  }


  void DeleteEnvironmentVariable(int idx) {
    auto Value = LoadedConfig->All(FEXCore::Config::ConfigOption::CONFIG_ENV);
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
        LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ENV, Environment);
        ImGui::CloseCurrentPopup();
        ConfigChanged = true;
      }

      ImGui::EndPopup();
    }
  }

  void FillEmulationConfig() {
    char RootFS[256]{};
    char ThunkHostPath[256]{};
    char ThunkGuestPath[256]{};
    char ThunkConfigPath[256]{};

    int NumEnvironmentVariables{};
    int NumRootFSPaths = NamedRootFS.size();

    if (ImGui::BeginTabItem("Emulation")) {
      auto Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_ROOTFS);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(RootFS, &(*Value)->at(0), 256);
      }
      ImGui::Text("Available named RootFS folders: %d", NumRootFSPaths);

      if (ImGui::ListBox("Named RootFS folders", &NamedRootFSSelected, NamedRootFSVariableFiller, nullptr, NumRootFSPaths)) {
        strncpy(RootFS, NamedRootFS.at(NamedRootFSSelected).c_str(), 256);
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_ROOTFS, RootFS);
        ConfigChanged = true;
      }

      if (ImGui::InputText("RootFS:", RootFS, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        NamedRootFSSelected = -1;
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_ROOTFS, RootFS);
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_THUNKHOSTLIBS);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(ThunkHostPath, &(*Value)->at(0), 256);
      }
      if (ImGui::InputText("Thunk Host library folder:", ThunkHostPath, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_THUNKHOSTLIBS, ThunkHostPath);
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_THUNKGUESTLIBS);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(ThunkGuestPath, &(*Value)->at(0), 256);
      }
      if (ImGui::InputText("Thunk Guest library folder:", ThunkGuestPath, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_THUNKGUESTLIBS, ThunkGuestPath);
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_THUNKCONFIG);
      if (Value.has_value() && !(*Value)->empty()) {
        strncpy(ThunkConfigPath, &(*Value)->at(0), 256);
      }
      if (ImGui::InputText("Thunk Config file:", ThunkConfigPath, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_THUNKCONFIG, ThunkConfigPath);
        ConfigChanged = true;
      }


      auto ValueList = LoadedConfig->All(FEXCore::Config::ConfigOption::CONFIG_ENV);
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
      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_O0);
      bool DisablePasses = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Disable Optimization Passes", &DisablePasses)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_O0, DisablePasses ? "1" : "0");
        ConfigChanged = true;
      }

      ImGui::EndTabItem();
    }
  }

  void FillLoggingConfig() {
    char LogFile[256]{};
    char IRDump[256]{};

    if (ImGui::BeginTabItem("Logging")) {
      auto Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_SILENTLOG);
      bool SilentLog = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Silent Logging", &SilentLog)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_SILENTLOG, SilentLog ? "1" : "0");
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
      auto Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_TSOENABLED);
      bool TSOEnabled = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("TSO Enabled", &TSOEnabled)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_TSOENABLED, TSOEnabled ? "1" : "0");
        ConfigChanged = true;
      }

      ImGui::Text("SMC Checks: ");
      int SMCChecks = FEXCore::Config::CONFIG_SMC_MMAN;

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_SMCCHECKS);
      if (Value.has_value()) {
        if (**Value == "0") {
          SMCChecks = FEXCore::Config::CONFIG_SMC_NONE;
        } else if (**Value == "1") {
          SMCChecks = FEXCore::Config::CONFIG_SMC_MMAN;
        } else if (**Value == "2") {
          SMCChecks = FEXCore::Config::CONFIG_SMC_FULL;
        }
      }

      bool SMCChanged = false;
      SMCChanged |= ImGui::RadioButton("None", &SMCChecks, FEXCore::Config::CONFIG_SMC_NONE); ImGui::SameLine();
      SMCChanged |= ImGui::RadioButton("MMan", &SMCChecks, FEXCore::Config::CONFIG_SMC_MMAN); ImGui::SameLine();
      SMCChanged |= ImGui::RadioButton("Full", &SMCChecks, FEXCore::Config::CONFIG_SMC_FULL);

      if (SMCChanged) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_SMCCHECKS, std::to_string(SMCChecks));
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_ABILOCALFLAGS);
      bool UnsafeLocalFlags = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Unsafe local flags optimization", &UnsafeLocalFlags)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_ABILOCALFLAGS, UnsafeLocalFlags ? "1" : "0");
        ConfigChanged = true;
      }

      Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_ABINOPF);
      bool NoPFCalculation = Value.has_value() && **Value == "1";
      if (ImGui::Checkbox("Disable PF calculation", &NoPFCalculation)) {
        LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_ABINOPF, NoPFCalculation ? "1" : "0");
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

  void DrawUI() {
    ImGuiIO& io = ImGui::GetIO();
    auto current_time = std::chrono::high_resolution_clock::now();
    auto Diff = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - GlobalTime);
    io.DeltaTime = Diff.count() > 0 ? Diff.count() : 1.0f/60.0f;
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

      if (ImGui::IsKeyPressed(SDL_SCANCODE_E) && io.KeyCtrl) {
        ImGui::OpenPopup(SavedPopupAppName);
      }

      ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x / 2, viewport->Pos.y + viewport->Size.y / 2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
      if (ImGui::BeginPopupModal(SavedPopupAppName)) {
        if (ImGui::InputText("App name", AppName, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
          std::string AppNameString = AppName;
          std::string Filename = FEXCore::Config::GetApplicationConfig(AppNameString, false);
          SaveFile(Filename);
          ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
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
        (ImGui::IsKeyPressed(SDL_SCANCODE_O) && io.KeyCtrl && !io.KeyShift)) {
      SelectedOpenFile = true;
    }
    if (Selected.OpenDefault ||
        (ImGui::IsKeyPressed(SDL_SCANCODE_O) && io.KeyCtrl && io.KeyShift)) {
      OpenFile(FEXCore::Config::GetConfigFileLocation());
      LoadNamedRootFSFolder();
      SetupINotify();
    }
    if (Selected.LoadDefault ||
        (ImGui::IsKeyPressed(SDL_SCANCODE_D) && io.KeyCtrl && io.KeyShift)) {
      LoadDefaultSettings();
      LoadNamedRootFSFolder();
      SetupINotify();
    }

    if (Selected.Save ||
        (ImGui::IsKeyPressed(SDL_SCANCODE_S) && io.KeyCtrl && !io.KeyShift)) {
      SaveFile(ConfigFilename);
    }
    if (Selected.SaveAs ||
        (ImGui::IsKeyPressed(SDL_SCANCODE_S) && io.KeyCtrl && io.KeyShift)) {
      SelectedSaveFileAs = true;
    }

    if (Selected.SaveDefault ||
        (ImGui::IsKeyPressed(SDL_SCANCODE_P) && io.KeyCtrl && io.KeyShift)) {
      SaveFile(FEXCore::Config::GetConfigFileLocation());
    }
    if (Selected.Close ||
        (ImGui::IsKeyPressed(SDL_SCANCODE_W) && io.KeyCtrl && !io.KeyShift)) {
      CloseConfig();
      ShutdownINotify();
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
  std::string ImGUIConfig = FEXCore::Config::GetConfigDirectory(false) + "FEXConfig_imgui.ini";

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
  {
      printf("Error: %s\n", SDL_GetError());
      return -1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window* window = SDL_CreateWindow("#FEXConfig", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 640, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
  io.IniFilename = &ImGUIConfig.at(0);

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  const char* glsl_version = "#version 130";
  ImGui_ImplOpenGL3_Init(glsl_version);

  GlobalTime = std::chrono::high_resolution_clock::now();

  bool Done{};
  while (!Done) {
    SDL_Event event;
    if (SDL_WaitEvent(nullptr))
    {
      while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
          Done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
          Done = true;
      }
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    DrawUI();

    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
      SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(window);
  }

  ShutdownINotify();

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
