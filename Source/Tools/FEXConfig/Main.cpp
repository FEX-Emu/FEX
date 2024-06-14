// SPDX-License-Identifier: MIT
#include "Common/Config.h"
#include "Common/FileFormatCheck.h"
#include "FEXCore/Config/Config.h"
#include "FEXCore/Utils/EnumUtils.h"
#include "Tools/CommonGUI/IMGui.h"

#include <FEXCore/Utils/Event.h>
#include <FEXCore/fextl/string.h>

#include <map>
#include <memory>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fextl {
// Helper to convert a std::filesystem::path to a fextl::string.
inline fextl::string string_from_path(const std::filesystem::path& Path) {
  return Path.string().c_str();
}
} // namespace fextl
namespace {
static std::chrono::time_point<std::chrono::high_resolution_clock> GlobalTime {};

static bool ConfigOpen {};
static bool ConfigChanged {};
static int EnvironmentVariableSelected {};
static int HostEnvironmentVariableSelected {};
static int NamedRootFSSelected {-1};

static fextl::string ConfigFilename {};
static fextl::unique_ptr<FEXCore::Config::Layer> LoadedConfig {};

static const char EnvironmentPopupName[] = "#New Environment Variable";
static const char HostEnvironmentPopupName[] = "#New Host Environment Variable";
static const char SavedPopupAppName[] = "#SavedApp";
static const char OpenedPopupAppName[] = "#OpenedApp";

static bool OpenMsgPopup {};
static bool SaveMsgIsOpen {};
static std::string MsgMessage {};
static const char MsgPopupName[] = "#Msg";
static std::chrono::high_resolution_clock::time_point MsgTimerStart {};

static bool SelectedOpenFile {};
static bool SelectedSaveFileAs {};
static ImGuiFs::Dialog DialogSaveAs {};
static ImGuiFs::Dialog DialogOpen {};

// Named rootfs
static std::vector<std::string> NamedRootFS {};
static std::mutex NamedRootFSUpdator {};

static std::atomic<int> INotifyFD {-1};
static int INotifyFolderFD {};
static std::thread INotifyThreadHandle {};
static std::atomic_bool INotifyShutdown {};

void OpenMsgMessagePopup(fextl::string Message) {
  OpenMsgPopup = true;
  MsgMessage = Message;
  MsgTimerStart = std::chrono::high_resolution_clock::now();
  FEX::GUI::HadUpdate();
}

void LoadDefaultSettings() {
  ConfigOpen = true;
  ConfigFilename = {};
  LoadedConfig = fextl::make_unique<FEX::Config::EmptyMapper>();
#define OPT_BASE(type, group, enum, json, default) LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(default));
#define OPT_STR(group, enum, json, default) LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, default);
#define OPT_STRARRAY(group, enum, json, default) // Do nothing
#define OPT_STRENUM(group, enum, json, default) \
  LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(FEXCore::ToUnderlying(default)));
#include <FEXCore/Config/ConfigValues.inl>

  // Erase unnamed options which shouldn't be set
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_IS_INTERPRETER);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_INTERPRETER_INSTALLED);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_APP_FILENAME);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_APP_CONFIG_NAME);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_IS64BIT_MODE);
}

bool OpenFile(fextl::string Filename, bool LoadDefault = false) {
  std::error_code ec {};
  if (!std::filesystem::exists(Filename, ec)) {
    if (LoadDefault) {
      LoadDefaultSettings();
      ConfigFilename = Filename;
      OpenMsgMessagePopup("Opened with default options: " + Filename);
      return true;
    }
    OpenMsgMessagePopup("Couldn't open: " + Filename);
    return false;
  }
  ConfigOpen = true;
  ConfigFilename = Filename;
  LoadedConfig = FEX::Config::CreateMainLayer(&Filename);
  LoadedConfig->Load();

  // Load default options and only overwrite only if the option didn't exist
#define OPT_BASE(type, group, enum, json, default)                                                 \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) {                 \
    LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(default)); \
  }
#define OPT_STR(group, enum, json, default)                                        \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) { \
    LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_##enum, default); \
  }
#define OPT_STRARRAY(group, enum, json, default) // Do nothing
#define OPT_STRENUM(group, enum, json, default)                                                                           \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) {                                        \
    LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(FEXCore::ToUnderlying(default))); \
  }
#include <FEXCore/Config/ConfigValues.inl>

  // Erase unnamed options which shouldn't be set
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_IS_INTERPRETER);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_INTERPRETER_INSTALLED);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_APP_FILENAME);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_APP_CONFIG_NAME);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_IS64BIT_MODE);

  return true;
}

void LoadNamedRootFSFolder() {
  std::scoped_lock<std::mutex> lk {NamedRootFSUpdator};
  NamedRootFS.clear();
  fextl::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
  std::error_code ec {};
  if (!std::filesystem::exists(RootFS, ec)) {
    // Doesn't exist, create the the folder as a user convenience
    if (!std::filesystem::create_directories(RootFS, ec)) {
      // Well I guess we failed
      return;
    }
  }
  for (auto& it : std::filesystem::directory_iterator(RootFS)) {
    if (it.is_directory()) {
      NamedRootFS.emplace_back(it.path().filename());
    } else if (it.is_regular_file()) {
      // If it is a regular file then we need to check if it is a valid archive
      if (it.path().extension() == ".sqsh" && FEX::FormatCheck::IsSquashFS(fextl::string_from_path(it.path()))) {
        NamedRootFS.emplace_back(it.path().filename());
      } else if (it.path().extension() == ".ero" && FEX::FormatCheck::IsEroFS(fextl::string_from_path(it.path()))) {
        NamedRootFS.emplace_back(it.path().filename());
      }
    }
  }
  std::sort(NamedRootFS.begin(), NamedRootFS.end());
}

void INotifyThread() {
  while (!INotifyShutdown) {
    constexpr size_t DATA_SIZE = (16 * (sizeof(struct inotify_event) + NAME_MAX + 1));
    char buf[DATA_SIZE];
    int Ret {};
    do {
      fd_set Set {};
      FD_ZERO(&Set);
      FD_SET(INotifyFD, &Set);
      struct timeval tv {};
      // 50 ms
      tv.tv_usec = 50000;
      Ret = select(INotifyFD + 1, &Set, nullptr, nullptr, &tv);
    } while (Ret == 0 && INotifyFD != -1);

    if (Ret == -1 || INotifyFD == -1) {
      // Just return on error
      INotifyShutdown = true;
      return;
    }

    // Spin through the events, we don't actually care what they are
    while (read(INotifyFD, buf, DATA_SIZE) > 0)
      ;

    // Now update the named vector
    LoadNamedRootFSFolder();

    FEX::GUI::HadUpdate();
  }
}

void SetupINotify() {
  if (INotifyFD != -1) {
    // Already setup
    return;
  }

  INotifyFD = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  INotifyShutdown = false;

  fextl::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
  INotifyFolderFD = inotify_add_watch(INotifyFD, RootFS.c_str(), IN_CREATE | IN_DELETE);
  if (INotifyFolderFD != -1) {
    INotifyThreadHandle = std::thread(INotifyThread);
  }
}

void ShutdownINotify() {
  close(INotifyFD);
  INotifyFD = -1;
  if (INotifyThreadHandle.joinable()) {
    INotifyThreadHandle.join();
  }
}

void SaveFile(fextl::string Filename) {
  if (SaveMsgIsOpen) {
    // Don't try saving a file while the message is already open.
    // Stops us from spam saving the file to the filesystem.
    return;
  }

  if (!ConfigOpen) {
    OpenMsgMessagePopup("Can't save file when config isn't open");
    return;
  }

  FEX::Config::SaveLayerToJSON(Filename, LoadedConfig.get());
  ConfigChanged = false;
  ConfigFilename = Filename;
  OpenMsgMessagePopup("Config Saved to: '" + Filename + "'");
  SaveMsgIsOpen = true;

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
  char BlockSize[32] {};

  if (ImGui::BeginTabItem("CPU")) {
    std::optional<fextl::string*> Value {};
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

    ImGui::EndTabItem();
  }
}

template<FEXCore::Config::ConfigOption Option>
bool EnvironmentVariableFiller(void* data, int idx, const char** out_text) {
  static char TmpString[256];
  auto Value = LoadedConfig->All(Option);
  if (Value.has_value()) {
    auto List = (*Value);
    auto it = List->begin();

    // Since this is a list, we don't have a linear allocator that we can just jump to an element
    // Just do a quick spin
    for (int i = 0; i < idx; ++i) {
      ++it;
    }

    snprintf(TmpString, 256, "%s", it->c_str());
    *out_text = TmpString;

    return true;
  }

  return false;
}

bool NamedRootFSVariableFiller(void* data, int idx, const char** out_text) {
  std::scoped_lock<std::mutex> lk {NamedRootFSUpdator};
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


template<FEXCore::Config::ConfigOption Option>
void DeleteEnvironmentVariable(int idx) {
  auto Value = LoadedConfig->All(Option);
  auto List = (*Value);
  auto it = List->begin();

  // Since this is a list, we don't have a linear allocator that we can just jump to an element
  // Just do a quick spin
  for (int i = 0; i < idx; ++i) {
    ++it;
  }

  List->erase(it);
  ConfigChanged = true;
}

void AddNewEnvironmentVariable() {
  char Environment[256] {};
  char HostEnvironment[256] {};

  if (ImGui::BeginPopup(EnvironmentPopupName)) {
    if (ImGui::InputText("New Environment", Environment, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
      LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_ENV, Environment);
      ImGui::CloseCurrentPopup();
      ConfigChanged = true;
    }

    ImGui::EndPopup();
  }

  ImGui::PushID(1);
  if (ImGui::BeginPopup(HostEnvironmentPopupName)) {
    if (ImGui::InputText("New Environment", HostEnvironment, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
      LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_HOSTENV, HostEnvironment);
      ImGui::CloseCurrentPopup();
      ConfigChanged = true;
    }

    ImGui::EndPopup();
  }
  ImGui::PopID();
}

void FillEmulationConfig() {
  char RootFS[256] {};
  char ThunkHostPath[256] {};
  char ThunkGuestPath[256] {};
  char ThunkConfigPath[256] {};

  int NumEnvironmentVariables {};
  int NumHostEnvironmentVariables {};
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
    auto ValueHostList = LoadedConfig->All(FEXCore::Config::ConfigOption::CONFIG_HOSTENV);
    if (ValueList.has_value()) {
      NumEnvironmentVariables = (*ValueList)->size();
    }

    if (ValueHostList.has_value()) {
      NumHostEnvironmentVariables = (*ValueHostList)->size();
    }

    ImGui::Text("Number of environment variables: %d", NumEnvironmentVariables);

    ImGui::ListBox("Environment variables", &EnvironmentVariableSelected,
                   EnvironmentVariableFiller<FEXCore::Config::ConfigOption::CONFIG_ENV>, nullptr, NumEnvironmentVariables);

    if (ImGui::SmallButton("+")) {
      ImGui::OpenPopup(EnvironmentPopupName);
    }

    if (NumEnvironmentVariables) {
      ImGui::SameLine();
      if (ImGui::SmallButton("-")) {
        DeleteEnvironmentVariable<FEXCore::Config::ConfigOption::CONFIG_ENV>(EnvironmentVariableSelected);
        EnvironmentVariableSelected = std::max(0, EnvironmentVariableSelected - 1);
      }
    }


    ImGui::PushID(1);
    ImGui::Text("Number of Host environment variables: %d", NumHostEnvironmentVariables);

    ImGui::ListBox("Host Env variables", &HostEnvironmentVariableSelected,
                   EnvironmentVariableFiller<FEXCore::Config::ConfigOption::CONFIG_HOSTENV>, nullptr, NumHostEnvironmentVariables);

    if (ImGui::SmallButton("+")) {
      ImGui::OpenPopup(HostEnvironmentPopupName);
    }

    if (NumHostEnvironmentVariables) {
      ImGui::SameLine();
      if (ImGui::SmallButton("-")) {
        DeleteEnvironmentVariable<FEXCore::Config::ConfigOption::CONFIG_HOSTENV>(HostEnvironmentVariableSelected);
        HostEnvironmentVariableSelected = std::max(0, HostEnvironmentVariableSelected - 1);
      }
    }
    ImGui::PopID();

    // Only draws if popup is open
    AddNewEnvironmentVariable();

    ImGui::Text("Debugging:");
    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_O0);
    bool DisablePasses = Value.has_value() && **Value == "1";
    if (ImGui::Checkbox("Disable Optimization Passes", &DisablePasses)) {
      LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_O0, DisablePasses ? "1" : "0");
      ConfigChanged = true;
    }

    ImGui::Text("Ahead Of Time JIT Options:");
    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_AOTIRGENERATE);
    bool AOTGenerate = Value.has_value() && **Value == "1";
    if (ImGui::Checkbox("Generate", &AOTGenerate)) {
      LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_AOTIRGENERATE, AOTGenerate ? "1" : "0");
      ConfigChanged = true;
    }

    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_AOTIRCAPTURE);
    bool AOTCapture = Value.has_value() && **Value == "1";
    if (ImGui::Checkbox("Capture", &AOTCapture)) {
      LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_AOTIRCAPTURE, AOTCapture ? "1" : "0");
      ConfigChanged = true;
    }

    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_AOTIRLOAD);
    bool AOTLoad = Value.has_value() && **Value == "1";
    if (ImGui::Checkbox("Load", &AOTLoad)) {
      LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_AOTIRLOAD, AOTLoad ? "1" : "0");
      ConfigChanged = true;
    }

    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_CACHEOBJECTCODECOMPILATION);

    ImGui::Text("Cache JIT object code:");
    int CacheJITObjectCode = 0;

    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_CACHEOBJECTCODECOMPILATION);
    if (Value.has_value()) {
      if (**Value == "0") {
        CacheJITObjectCode = FEXCore::Config::ConfigObjectCodeHandler::CONFIG_NONE;
      } else if (**Value == "1") {
        CacheJITObjectCode = FEXCore::Config::ConfigObjectCodeHandler::CONFIG_READ;
      } else if (**Value == "2") {
        CacheJITObjectCode = FEXCore::Config::ConfigObjectCodeHandler::CONFIG_READWRITE;
      }
    }

    bool CacheChanged = false;
    CacheChanged |= ImGui::RadioButton("Off", &CacheJITObjectCode, FEXCore::Config::ConfigObjectCodeHandler::CONFIG_NONE);
    ImGui::SameLine();
    CacheChanged |= ImGui::RadioButton("Read-only", &CacheJITObjectCode, FEXCore::Config::ConfigObjectCodeHandler::CONFIG_READ);
    ImGui::SameLine();
    CacheChanged |= ImGui::RadioButton("Read/Write", &CacheJITObjectCode, FEXCore::Config::ConfigObjectCodeHandler::CONFIG_READWRITE);

    if (CacheChanged) {
      LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_CACHEOBJECTCODECOMPILATION, std::to_string(CacheJITObjectCode));
      ConfigChanged = true;
    }

    ImGui::EndTabItem();
  }
}

void FillLoggingConfig() {
  char LogFile[256] {};
  char IRDump[256] {};

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
    auto VectorTSO = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_VECTORTSOENABLED);
    auto MemcpyTSO = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_MEMCPYSETTSOENABLED);
    auto HalfBarrierTSO = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_HALFBARRIERTSOENABLED);

    bool TSOEnabled = Value.has_value() && **Value == "1";
    bool VectorTSOEnabled = VectorTSO.has_value() && **VectorTSO == "1";
    bool MemcpyTSOEnabled = MemcpyTSO.has_value() && **MemcpyTSO == "1";
    bool HalfBarrierTSOEnabled = HalfBarrierTSO.has_value() && **HalfBarrierTSO == "1";

    if (ImGui::Checkbox("TSO Emulation Enabled", &TSOEnabled)) {
      LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_TSOENABLED, TSOEnabled ? "1" : "0");
      ConfigChanged = true;
    }

    if (TSOEnabled) {
      if (ImGui::TreeNodeEx("TSO Emulation sub-options", ImGuiTreeNodeFlags_Leaf)) {
        if (ImGui::Checkbox("Vector TSO Emulation Enabled", &VectorTSOEnabled)) {
          LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_VECTORTSOENABLED, VectorTSOEnabled ? "1" : "0");
          ConfigChanged = true;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::Text("Disables TSO emulation on vector load/store instructions");
          ImGui::EndTooltip();
        }

        if (ImGui::Checkbox("Memcpy TSO Emulation Enabled", &MemcpyTSOEnabled)) {
          LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_MEMCPYSETTSOENABLED, MemcpyTSOEnabled ? "1" : "0");
          ConfigChanged = true;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::Text("Disables TSO emulation on memcpy/memset instructions");
          ImGui::EndTooltip();
        }

        if (ImGui::Checkbox("Unaligned Half-Barrier TSO Emulation Enabled", &HalfBarrierTSOEnabled)) {
          LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_HALFBARRIERTSOENABLED, HalfBarrierTSOEnabled ? "1" : "0");
          ConfigChanged = true;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::Text("Disables half-barrier TSO emulation on unaligned load/store instructions");
          ImGui::EndTooltip();
        }

        ImGui::TreePop();
      }
    }

    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_PARANOIDTSO);
    bool ParanoidTSOEnabled = Value.has_value() && **Value == "1";
    if (ImGui::Checkbox("Paranoid TSO Enabled", &ParanoidTSOEnabled)) {
      LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_PARANOIDTSO, ParanoidTSOEnabled ? "1" : "0");
      ConfigChanged = true;
    }

    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_X87REDUCEDPRECISION);
    bool X87ReducedPrecision = Value.has_value() && **Value == "1";
    if (ImGui::Checkbox("X87 Reduced Precision", &X87ReducedPrecision)) {
      LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_X87REDUCEDPRECISION, X87ReducedPrecision ? "1" : "0");
      ConfigChanged = true;
    }

    ImGui::Text("SMC Checks: ");
    int SMCChecks = FEXCore::Config::CONFIG_SMC_MTRACK;

    Value = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_SMCCHECKS);
    if (Value.has_value()) {
      if (**Value == "0") {
        SMCChecks = FEXCore::Config::CONFIG_SMC_NONE;
      } else if (**Value == "1") {
        SMCChecks = FEXCore::Config::CONFIG_SMC_MTRACK;
      } else if (**Value == "2") {
        SMCChecks = FEXCore::Config::CONFIG_SMC_FULL;
      }
    }

    bool SMCChanged = false;
    SMCChanged |= ImGui::RadioButton("None", &SMCChecks, FEXCore::Config::CONFIG_SMC_NONE);
    ImGui::SameLine();
    SMCChanged |= ImGui::RadioButton("MTrack (Default)", &SMCChecks, FEXCore::Config::CONFIG_SMC_MTRACK);
    ImGui::SameLine();
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

    ImGui::EndTabItem();
  }
}

static const std::map<FEXCore::Config::ConfigOption, std::string> ConfigToNameLookup = {{
#define OPT_BASE(type, group, enum, json, default) {FEXCore::Config::ConfigOption::CONFIG_##enum, #json},
#include <FEXCore/Config/ConfigValues.inl>
}};

struct TmpString {
  char Str[256];
  std::string Name;
};
static std::vector<std::vector<TmpString>> AdvancedOptions {};

void UpdateAdvancedOptionsVector() {
  AdvancedOptions.clear();
  // Push everything in to our vector table that we can modify instead of the map
  auto Options = LoadedConfig->GetOptionMap();
  AdvancedOptions.resize(Options.size());

  size_t i = 0;
  for (auto& Option : Options) {
    auto ConfigName = ConfigToNameLookup.find(Option.first);
    auto& AdvancedOption = AdvancedOptions.at(i);
    AdvancedOption.resize(Option.second.size());
    size_t j = 0;
    for (auto& OptionList : Option.second) {
      strcpy(AdvancedOption[j].Str, OptionList.c_str());
      AdvancedOption[j].Name = ConfigName->second;
      if (Option.second.size() > 1) {
        AdvancedOption[j].Name += " " + std::to_string(j);
      }
      ++j;
    }
    ++i;
  }
}

void FillAdvancedConfig() {
  if (ImGui::BeginTabItem("Advanced")) {
    auto Options = LoadedConfig->GetOptionMap();

    if (ImGui::SmallButton("Refresh") || AdvancedOptions.size() != Options.size()) {
      UpdateAdvancedOptionsVector();
    }

    if (Options.size()) {
      ImGui::Columns(2);
      size_t i = 0;
      for (auto& Option : Options) {
        auto ConfigName = ConfigToNameLookup.find(Option.first);
        ImGui::Text("%s", ConfigName->second.c_str());
        ImGui::NextColumn();

        auto& AdvancedOption = AdvancedOptions.at(i);
        size_t j = 0;

        bool Stop = false;
        for (auto& OptionList : AdvancedOption) {
          // ImGui::Text("%s", OptionList.Str);
          if (ImGui::InputText(OptionList.Name.c_str(), OptionList.Str, sizeof(TmpString), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (Option.second.size() == 1) {
              LoadedConfig->EraseSet(Option.first, OptionList.Str);
            } else {
              auto& All = Option.second;
              auto Iter = All.begin();
              std::advance(Iter, j);
              *Iter = OptionList.Str;

              LoadedConfig->Erase(Option.first);
              for (auto& Value : All) {
                LoadedConfig->Set(Option.first, Value);
              }
            }
            ConfigChanged = true;
            UpdateAdvancedOptionsVector();
          }

          ImGui::SameLine();

          ImGui::PushID(OptionList.Name.c_str());
          if (ImGui::SmallButton("-")) {
            if (Option.second.size() == 1) {
              LoadedConfig->Erase(Option.first);
            } else {
              auto& All = Option.second;
              auto Iter = All.begin();
              std::advance(Iter, j);
              All.erase(Iter);

              LoadedConfig->Erase(Option.first);
              for (auto& Value : All) {
                LoadedConfig->Set(Option.first, Value);
              }
            }
            Stop = true;
            ConfigChanged = true;
            UpdateAdvancedOptionsVector();
          }
          ImGui::PopID();

          ++j;
        }

        ImGui::NextColumn();
        ++i;
        if (Stop) {
          break;
        }
      }
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
  FillAdvancedConfig();
  ImGui::EndTabBar();
}

bool DrawUI() {
  ImGuiIO& io = ImGui::GetIO();
  auto current_time = std::chrono::high_resolution_clock::now();
  auto Diff = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - GlobalTime);
  io.DeltaTime = Diff.count() > 0 ? Diff.count() : 1.0f / 60.0f;
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

  struct {
    bool Open {};
    bool OpenDefault {};
    bool OpenAppProfile {};
    bool Save {};
    bool SaveAs {};
    bool SaveDefault {};
    bool Close {};
    bool Quit {};
  } Selected;

  char AppName[256] {};

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("Open", "CTRL+O", &Selected.Open, true);
      ImGui::MenuItem("Open from default location", "CTRL+SHIFT+O", &Selected.OpenDefault, true);
      ImGui::MenuItem("Open App profile", "CTRL+I", &Selected.OpenAppProfile, true);

      ImGui::MenuItem("Save", "CTRL+S", &Selected.Save, true);
      ImGui::MenuItem("Save As", "CTRL+SHIFT+S", &Selected.SaveAs, true);
      ImGui::MenuItem("Save As App profile", "CTRL+E", nullptr, true);
      ImGui::MenuItem("Save Default", "CTRL+SHIFT+P", &Selected.SaveDefault, true);

      ImGui::MenuItem("Close", "CTRL+W", &Selected.Close, true);
      ImGui::MenuItem("Quit", "CTRL+Q", &Selected.Quit, true);

      ImGui::EndMenu();
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight());

    if (ConfigOpen) {
      if (ConfigChanged) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0, 1.0, 0.0, 1.0));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0, 1.0, 0.0, 1.0));
      } else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0, 1.0, 0.0, 1.0));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0, 1.0, 0.0, 1.0));
      }
    } else {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0, 0.0, 0.0, 1.0));
      ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0, 0.0, 0.0, 1.0));
    }

    ImGui::RadioButton("", true);
    ImGui::PopStyleColor(2);

    ImGui::EndMenuBar();
  }

  if (ConfigOpen) {
    if (ImGui::BeginChild("#Config")) {
      FillConfigWindow();
    }

    if (ImGui::IsKeyPressed(SDL_SCANCODE_E) && io.KeyCtrl && !io.KeyShift) {
      ImGui::OpenPopup(SavedPopupAppName);
    }

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x / 2, viewport->Pos.y + viewport->Size.y / 2), ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(SavedPopupAppName)) {
      ImGui::SetKeyboardFocusHere();
      if (ImGui::InputText("App name", AppName, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        fextl::string AppNameString = AppName;
        fextl::string Filename = FEXCore::Config::GetApplicationConfig(AppNameString, false);
        SaveFile(Filename);
        ImGui::CloseCurrentPopup();
      }

      if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::EndChild();
  }

  // Need this frame delay loop since ImGui doesn't allow us to enable a popup near the end of the frame
  if (OpenMsgPopup) {
    ImGui::OpenPopup(MsgPopupName);
    OpenMsgPopup = false;
  }

  if (Selected.OpenAppProfile || (ImGui::IsKeyPressed(SDL_SCANCODE_I) && io.KeyCtrl && !io.KeyShift)) {
    ImGui::OpenPopup(OpenedPopupAppName);
  }

  // Center the saved popup in the center of the window
  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x / 2, viewport->Pos.y + viewport->Size.y / 2), ImGuiCond_Appearing,
                          ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopup(MsgPopupName)) {
    ImGui::Text("%s", MsgMessage.c_str());
    if ((std::chrono::high_resolution_clock::now() - MsgTimerStart) >= std::chrono::seconds(2)) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  } else if (SaveMsgIsOpen) {
    SaveMsgIsOpen = false;
  }

  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x / 2, viewport->Pos.y + viewport->Size.y / 2), ImGuiCond_Appearing,
                          ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal(OpenedPopupAppName)) {

    ImGui::SetKeyboardFocusHere();
    if (ImGui::InputText("App name", AppName, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
      fextl::string AppNameString = AppName;
      fextl::string Filename = FEXCore::Config::GetApplicationConfig(AppNameString, false);
      OpenFile(Filename, false);
      ImGui::CloseCurrentPopup();
    }

    if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  if (Selected.Open || (ImGui::IsKeyPressed(SDL_SCANCODE_O) && io.KeyCtrl && !io.KeyShift)) {
    SelectedOpenFile = true;
  }
  if (Selected.OpenDefault || (ImGui::IsKeyPressed(SDL_SCANCODE_O) && io.KeyCtrl && io.KeyShift)) {
    if (OpenFile(FEXCore::Config::GetConfigFileLocation(), true)) {
      LoadNamedRootFSFolder();
      SetupINotify();
    }
  }

  if (Selected.Save || (ImGui::IsKeyPressed(SDL_SCANCODE_S) && io.KeyCtrl && !io.KeyShift)) {
    SaveFile(ConfigFilename);
  }
  if (Selected.SaveAs || (ImGui::IsKeyPressed(SDL_SCANCODE_S) && io.KeyCtrl && io.KeyShift)) {
    SelectedSaveFileAs = true;
  }

  if (Selected.SaveDefault || (ImGui::IsKeyPressed(SDL_SCANCODE_P) && io.KeyCtrl && io.KeyShift)) {
    SaveFile(FEXCore::Config::GetConfigFileLocation());
  }
  if (Selected.Close || (ImGui::IsKeyPressed(SDL_SCANCODE_W) && io.KeyCtrl && !io.KeyShift)) {
    CloseConfig();
    ShutdownINotify();
  }

  if (Selected.Quit || (ImGui::IsKeyPressed(SDL_SCANCODE_Q) && io.KeyCtrl && !io.KeyShift)) {
    Selected.Quit = true;
  }

  ImGui::End(); // End dockspace

  const char* InitialPath;
  const char* File;

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

  // Return true to keep rendering
  return !Selected.Quit;
}
} // namespace

int main(int argc, char** argv) {
  FEX::Config::InitializeConfigs();

  fextl::string ImGUIConfig = FEXCore::Config::GetConfigDirectory(false) + "FEXConfig_imgui.ini";
  auto [window, gl_context] = FEX::GUI::SetupIMGui("#FEXConfig", ImGUIConfig);

  GlobalTime = std::chrono::high_resolution_clock::now();

  // Attempt to open the config passed in
  if (argc > 1) {
    if (OpenFile(argv[1], true)) {
      LoadNamedRootFSFolder();
      SetupINotify();
    }
  } else {
    if (OpenFile(FEXCore::Config::GetConfigFileLocation(), true)) {
      LoadNamedRootFSFolder();
      SetupINotify();
    }
  }

  FEX::GUI::DrawUI(window, DrawUI);

  ShutdownINotify();

  // Cleanup
  FEX::GUI::Shutdown(window, gl_context);
  return 0;
}
