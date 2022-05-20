#include "Common/Config.h"
#include "Common/SocketLogging.h"

#include "Tools/CommonGUI/IMGui.h"

#include <map>
#include <unordered_set>
#include <unistd.h>
#include <vector>

namespace Common {
  const char *GetCharLevel(uint32_t Level) {
    switch (Level) {
    case LogMan::NONE:
      return "NONE";
      break;
    case LogMan::ASSERT:
      return "ASSERT";
      break;
    case LogMan::ERROR:
      return "ERROR";
      break;
    case LogMan::DEBUG:
      return "DEBUG";
      break;
    case LogMan::INFO:
      return "Info";
      break;
    case LogMan::STDOUT:
      return "STDOUT";
      break;
    case LogMan::STDERR:
      return "STDERR";
      break;
    default:
      return "???";
      break;
    }
  }
}

namespace CLI {
  static void MsgHandler(int FD, uint64_t Timestamp, uint32_t PID, uint32_t TID, uint32_t Level, const char* Msg) {
    auto CharLevel = Common::GetCharLevel(Level);
    const auto Output = fmt::format("[{}][{}][{}.{}] {}\n", CharLevel, Timestamp, PID, TID, Msg);
    write(STDERR_FILENO, Output.c_str(), Output.size());
  }
}

namespace GUI {
  static std::chrono::time_point<std::chrono::high_resolution_clock> GlobalTime{};

  // Configuration
  static size_t MaxLogLines = 1000;
  static LogMan::DebugLevels LevelFilter = LogMan::DebugLevels::INFO;
  bool AlsoCLI {true};

  // PID information
  struct PIDMapping {
    std::unordered_set<uint32_t> FDs{};
    std::unordered_set<uint32_t> TIDs{};
  };
  // PID -> FD/TID mapping
  std::map<uint32_t, PIDMapping> PIDs{};
  std::mutex PIDMappingMutex{};

  // Log window
  static int LogSelected{};

  struct LogData {
    uint64_t Timestamp;
    uint32_t PID;
    uint32_t TID;
    uint32_t Level;
    int FD;
    std::string Log{};
  };

  std::mutex LogMutex{};
  std::vector<LogData> LogLines{};

  static void MsgHandler(int FD, uint64_t Timestamp, uint32_t PID, uint32_t TID, uint32_t Level, const char* Msg) {
    if (Level > LevelFilter) {
      // Skip this log
      return;
    }

    {
      std::unique_lock lk{PIDMappingMutex};
      auto it = PIDs.find(PID);
      if (it == PIDs.end()) {
        it = PIDs.try_emplace(PID, PIDMapping {
        }).first;
      }

      it->second.FDs.emplace(FD);
      it->second.TIDs.emplace(TID);
    }

    if (AlsoCLI) {
      CLI::MsgHandler(FD, Timestamp, PID, TID, Level, Msg);
    }

    std::unique_lock lk{LogMutex};
    auto CharLevel = Common::GetCharLevel(Level);
    LogLines.emplace_back(LogData {
        Timestamp,
        PID,
        TID,
        Level,
        FD,
        fmt::format("[{}][{}][{}.{}] {}\n", CharLevel, Timestamp, PID, TID, Msg),
    });

    if (LogLines.size() > MaxLogLines) {
      size_t NumberToRemove = LogLines.size() - MaxLogLines;
      LogLines.erase(LogLines.begin(), LogLines.begin()+NumberToRemove);
    }
    FEX::GUI::HadUpdate();
  }

  static void FDClosedHandler(int FD) {
    std::unique_lock lk{PIDMappingMutex};
    for (auto it = PIDs.begin(); it != PIDs.end(); ) {
      it->second.FDs.erase(FD);
      if (it->second.FDs.empty()) {
        it = PIDs.erase(it);
      }
      else {
        ++it;
      }
    }
    FEX::GUI::HadUpdate();
  }

  bool LoglineFiller(void *data, int idx, const char** out_text) {
    std::unique_lock lk{LogMutex};

    if (idx < LogLines.size()) {
      auto &Log = LogLines.at(idx);
      *out_text = Log.Log.c_str();

      return true;

    }
    return false;
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

    if (ImGui::BeginMenuBar()) {
      // XXX: Nothing here yet
      ImGui::EndMenuBar();
    }

    {
      if (ImGui::Begin("#Config")) {
        if (ImGui::RadioButton("Info", LevelFilter == LogMan::DebugLevels::INFO)) {
          LevelFilter = LogMan::DebugLevels::INFO;
        }
        if (ImGui::RadioButton("Debug", LevelFilter == LogMan::DebugLevels::DEBUG)) {
          LevelFilter = LogMan::DebugLevels::DEBUG;
        }
        if (ImGui::RadioButton("Error", LevelFilter == LogMan::DebugLevels::ERROR)) {
          LevelFilter = LogMan::DebugLevels::ERROR;
        }
        if (ImGui::RadioButton("Assert", LevelFilter == LogMan::DebugLevels::ASSERT)) {
          LevelFilter = LogMan::DebugLevels::ASSERT;
        }
        if (ImGui::RadioButton("None", LevelFilter == LogMan::DebugLevels::NONE)) {
          LevelFilter = LogMan::DebugLevels::NONE;
        }

        ImGui::Checkbox("Print to stderr", &AlsoCLI);

        if (ImGui::TreeNodeEx("PIDs", ImGuiTreeNodeFlags_DefaultOpen)) {
          std::unique_lock lk{PIDMappingMutex};
          for (const auto& it : PIDs) {
            ImGui::PushID(it.first);
            ImGui::Text("PID: %d", it.first);
            ImGui::Indent();
            for (auto TID : it.second.TIDs) {
              if (TID != it.first) {
                ImGui::Text("%d", TID);
              }
            }
            ImGui::Unindent();

            ImGui::PopID();
          }
          ImGui::TreePop();
        }

        ImGui::End();
      }
      if (ImGui::Begin("#Log")) {
        auto Region = ImGui::GetWindowContentRegionMax();
        ImGui::PushItemWidth(Region.x);
        const ImGuiStyle& style = ImGui::GetStyle();
        float FitableItems = Region.y / ImGui::GetTextLineHeightWithSpacing() - style.FramePadding.y * 2.0;

        ImGui::ListBox("##Log", &LogSelected, LoglineFiller, nullptr, LogLines.size(), FitableItems);
        ImGui::End();
      }
    }

    ImGui::End(); // End dockspace

    ImGui::Render();
  }
}

int main(int argc, char **argv, char **envp) {
  const char *Port = "8087";
  bool Graphical {};

  for (size_t i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-g") == 0) {
      Graphical = true;
    }
    else {
      Port = argv[i];
    }
  }

  auto Listener = FEX::SocketLogging::Server::StartListening(Port);

  if (!Graphical) {
    Listener->SetMsgHandler(CLI::MsgHandler);
    Listener->WaitForShutdown();
    return 0;
  }

  Listener->SetMsgHandler(GUI::MsgHandler);
  Listener->SetFDClosedHandler(GUI::FDClosedHandler);

  // Imgui based UI
  std::string ImGUIConfig = FEXCore::Config::GetConfigDirectory(false) + "FEXLogServer_imgui.ini";
  GUI::GlobalTime = std::chrono::high_resolution_clock::now();

  auto [window, gl_context] = FEX::GUI::SetupIMGui("#FEXLogServer", ImGUIConfig);
  FEX::GUI::DrawUI(window, GUI::DrawUI);
  FEX::GUI::Shutdown(window, gl_context);

  Listener->WaitForShutdown();

  return 0;
}
