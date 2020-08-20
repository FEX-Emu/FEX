#include "DebuggerState.h"
#include "Disassembler.h"
#include "FEXImGui.h"
#include "IMGui_I.h"
#include "LogManager.h"
#include "IRLexer.h"
#include "Common/Config.h"
#include "Common/StringUtil.h"
#include "Util/DataRingBuffer.h"

#include <chrono>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/ContextDebug.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

#include <imgui.h>
#include <imgui_internal.h>
#define YES_IMGUIFILESYSTEM
#include <addons/imgui_user.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iterator>
#include <list>
#include <vector>
#include <tiny-json.h>
#include <json-maker.h>

namespace FEX::Debugger {
double g_Time = 0.0;

constexpr size_t TmpSize = 512;
char Tmp[TmpSize];
namespace CPUState {
  bool ShowCPUState {false};
	bool ShowCPUThreads {true};
  FEXCore::Core::CPUState PreviousState;
  FEXCore::Core::CPUState CurrentState;

  int CurrentThreadSelected {0};

  bool ThreadNameGetter(void *data, int idx, const char** out_text) {
		if (FEX::DebuggerState::ActiveCore()) {
      snprintf(Tmp, TmpSize, "Thread %d", idx);
      *out_text = Tmp;
			return true;
		}
		return false;
	}

  void MenuItems() {
    ImGui::MenuItem("CPU State", nullptr, &ShowCPUState);
    ImGui::MenuItem("CPU Threads", nullptr, &ShowCPUThreads);

    if (ImGui::BeginMenu("Backend")) {
      if (ImGui::MenuItem("IR Interpreter", nullptr, FEX::DebuggerState::GetCoreType() == FEXCore::Config::ConfigCore::CONFIG_INTERPRETER, !FEX::DebuggerState::ActiveCore()))
        FEX::DebuggerState::SetCoreType(FEXCore::Config::ConfigCore::CONFIG_INTERPRETER);
      if (ImGui::MenuItem("IR JIT", nullptr, FEX::DebuggerState::GetCoreType() == FEXCore::Config::ConfigCore::CONFIG_IRJIT, !FEX::DebuggerState::ActiveCore()))
        FEX::DebuggerState::SetCoreType(FEXCore::Config::ConfigCore::CONFIG_IRJIT);
      ImGui::EndMenu();
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Single Step", nullptr, FEX::DebuggerState::GetRunningMode() == 1, !FEX::DebuggerState::IsCoreRunning()))
        FEX::DebuggerState::SetRunningMode(1);

    if (ImGui::MenuItem("Regular Running", nullptr, FEX::DebuggerState::GetRunningMode() == 0, !FEX::DebuggerState::IsCoreRunning()))
        FEX::DebuggerState::SetRunningMode(0);
  }

  void Windows() {
    if (ImGui::Begin("#CPU State", &ShowCPUState)) {
      auto &State = CurrentState;
      bool PushedCol = false;
      #define DiffCol(X) do { if (PreviousState.X != State.X) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0)); PushedCol = true;} } while (0)
      #define DiffPop(X) do { if (PushedCol) { ImGui::PopStyleColor(); PushedCol = false; if (ImGui::IsItemHovered()) ImGui::SetTooltip("Prev: 0x%lx", PreviousState.X);} } while (0)

      DiffCol(rip);
      ImGui::Text("RIP: 0x%lx", State.rip);
      DiffPop(rip);

      if (ImGui::CollapsingHeader("GPRs")) {
        const std::vector<const char*> GPRNames = {
          "RAX",
          "RBX",
          "RCX",
          "RDX",
          "RSI",
          "RDI",
          "RBP",
          "RSP",
          " R8",
          " R9",
          "R10",
          "R11",
          "R12",
          "R13",
          "R14",
          "R15",
        };

        for (unsigned i = 0; i < 16; ++i) {
          DiffCol(gregs[i]);
          ImGui::Text("%s: 0x%016lx", GPRNames[i], State.gregs[i]);
          DiffPop(gregs[i]);
        }
      }
      if (ImGui::CollapsingHeader("XMMs")) {
        for (unsigned i = 0; i < 16; ++i) {
          DiffCol(xmm[i][0]);
          ImGui::Text("%2d: 0x%016lx - 0x%016lx", i, State.xmm[i][0], State.xmm[i][1]);
          DiffPop(xmm[i][0]);
        }
      }
      if (ImGui::CollapsingHeader("FLAGs")) {
        // DiffCol(rflags);
        // ImGui::Text("EFLAG: 0x%x", static_cast<uint32_t>(State.rflags));
        // DiffPop(rflags);
        // #define PRINT_FLAG(X, Y) ImGui::Text(#X ": %d " #Y ": %d", !!(State.rflags & FEXCore::X86State::RFLAG_ ## X ## _LOC), !!(State.rflags & FEXCore::X86State::RFLAG_ ## Y ## _LOC))
        // PRINT_FLAG(CF, PF);
        // PRINT_FLAG(AF, ZF);
        // PRINT_FLAG(SF, OF);
        // ImGui::Separator();
        // PRINT_FLAG(TF, IF);
        // PRINT_FLAG(DF, IOPL);
        // PRINT_FLAG(NT, RF);
        // PRINT_FLAG(VM, AC);
        // PRINT_FLAG(VIF, VIP);
        // ImGui::Text("ID: %d", !!(State.rflags & FEXCore::X86State::RFLAG_ID_LOC));
        // #undef PRINT_FLAG
      }

      ImGui::Separator();
      DiffCol(fs);
      ImGui::Text("FS: 0x%lx", State.fs);
      DiffPop(fs);
      DiffCol(gs);
      ImGui::Text("GS: 0x%lx", State.gs);
      DiffPop(gs);
      #undef DiffCol
      #undef DiffPop

      FEX::DebuggerState::SetHasNewState(false);
    }
    ImGui::End();

		if (ImGui::Begin("#CPU Threads", &ShowCPUThreads)) {
			size_t ThreadCount = 0;
			if (FEX::DebuggerState::ActiveCore()) {
        ThreadCount = FEXCore::Context::Debug::GetThreadCount(FEX::DebuggerState::GetContext());
			}
			ImGui::Text("Threads: %ld", ThreadCount);
      ImGui::ListBox("##Threads", &CurrentThreadSelected, ThreadNameGetter, nullptr, ThreadCount);
		}
		ImGui::End();
  }

  void Status() {
    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight());

    bool PushedStyle = false;
    if (!FEX::DebuggerState::IsCoreRunning() ||
        FEX::DebuggerState::GetCoreCurrentRunningMode() == 0) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0, 0.0, 0.0, 1.0));
      ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0, 1.0, 0.0, 1.0));
      PushedStyle = true;
    }
    else if (FEX::DebuggerState::IsCoreRunning() &&
        FEX::DebuggerState::GetCoreCurrentRunningMode() == 1) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0, 1.0, 0.0, 1.0));
      ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0, 1.0, 0.0, 1.0));
      PushedStyle = true;
    }

    ImGui::RadioButton("", FEX::DebuggerState::IsCoreRunning());

    if (PushedStyle) {
      ImGui::PopStyleColor(2);
    }
  }

  void NewState() {
    PreviousState = CurrentState;
    CurrentState = FEX::DebuggerState::GetCPUState();
  }
}

// CPU stats window
namespace CPUStats {
  bool ShowCPUStats {true};
  FEX::Debugger::Util::DataRingBuffer<float> InstExecuted(60 * 10);
  FEX::Debugger::Util::DataRingBuffer<float> BlocksCompiled(60 * 10);
  auto LastTime = std::chrono::high_resolution_clock::now();

  void Window() {
    auto Now = std::chrono::high_resolution_clock::now();
    if ((Now - LastTime) >= std::chrono::seconds(1)) {
      LastTime = Now;
      if (FEX::DebuggerState::ActiveCore()) {
        auto RuntimeStats = FEXCore::Context::Debug::GetRuntimeStatsForThread(FEX::DebuggerState::GetContext(), CPUState::CurrentThreadSelected);
        InstExecuted.push_back(RuntimeStats->InstructionsExecuted);
        BlocksCompiled.push_back(RuntimeStats->BlocksCompiled);
        RuntimeStats->InstructionsExecuted = 0;
        RuntimeStats->BlocksCompiled = 0;
      }
    }

    if (ImGui::Begin("#CPU Stats", &ShowCPUStats)) {
      if (!InstExecuted.empty()) {
        ImGui::PlotHistogram("MIPS", InstExecuted(), InstExecuted.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 50));
        ImGui::SameLine();
        ImGui::Text("%f", InstExecuted.back());
      }

      if (!BlocksCompiled.empty()) {
        ImGui::PlotHistogram("Blocks Compiled", BlocksCompiled(), BlocksCompiled.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 50));
        ImGui::SameLine();
        ImGui::Text("%f", BlocksCompiled.back());
      }

    }
    ImGui::End();
  }
}

namespace MemoryViewer {
  std::vector<FEXCore::Memory::MemRegion> MappedRegions;
}

// Logging Windows
namespace Logging {
  bool ShowLogOutput = true;
  bool ShowLogstdout = true;
  bool ShowLogstderr = true;

  bool ScrollLogWindow = false;
  std::vector<std::string> LogData;
  std::vector<std::string> stdoutData;
  std::vector<std::string> stderrData;
  constexpr size_t MAX_LOG_SIZE = 10000;
  std::string IRData;

  void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
    const char *CharLevel{nullptr};

    switch (Level) {
    case LogMan::NONE:
      CharLevel = "NONE";
      break;
    case LogMan::ASSERT:
      CharLevel = "ASSERT";
      break;
    case LogMan::ERROR:
      CharLevel = "ERROR";
      break;
    case LogMan::DEBUG:
      CharLevel = "DEBUG";
      break;
    case LogMan::INFO:
      CharLevel = "Info";
      break;
    case LogMan::STDOUT:
      CharLevel = "stdout";
      break;
    case LogMan::STDERR:
      CharLevel = "stderr";
      break;

    default:
      CharLevel = "???";
      break;
    }

    auto AddTo = [](auto &Output, std::string &Log) {
      Output.emplace_back(Log);
      if (Output.size() > MAX_LOG_SIZE) {
        Output.erase(Output.begin(), Output.begin() + (Output.size() - MAX_LOG_SIZE));
      }
    };

    std::string Log;
    Log += "[";
    Log += CharLevel;
    Log += "] ";
    Log += Message;
    FEX::StringUtil::rtrim(Log);
    AddTo(LogData, Log);
    if (Level == LogMan::STDOUT)
      AddTo(stdoutData, Log);
    if (Level == LogMan::STDERR)
      AddTo(stderrData, Log);

    printf("[%s] %s\n", CharLevel, Message);
    ScrollLogWindow = true;
  }

  void AssertHandler(char const *Message) {
    std::string Log;
    Log += "[ASSERT] ";
    Log += Message;
    FEX::StringUtil::rtrim(Log);
    LogData.emplace_back(Log);

    if (LogData.size() > MAX_LOG_SIZE) {
      LogData.erase(LogData.begin(), LogData.begin() + (LogData.size() - MAX_LOG_SIZE));
    }
    printf("[ASSERT] %s\n", Message);
    ScrollLogWindow = true;
  }

  void LogWindow(bool *LogBool, const char *Name, std::vector<std::string> const &Log) {
      if (LogBool) {
        if (ImGui::Begin(Name, LogBool)) {
          ImGui::BeginChild("LogConsole");
          for (auto &Text : Log) {
            bool pop_color = false;
            if (strstr(Text.c_str(), "ERROR")) {
              pop_color = true;
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
            }
            if (strstr(Text.c_str(), "ASSERT")) {
              pop_color = true;
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            }
            if (strstr(Text.c_str(), "DEBUG")) {
              pop_color = true;
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.25f, 1.0f));
            }

            ImGui::TextUnformatted(Text.c_str());

            if (pop_color) {
              ImGui::PopStyleColor();
            }
          }
          if (ScrollLogWindow) {
            ImGui::SetScrollHereY(1.0f);
            ScrollLogWindow = false;
          }
          ImGui::EndChild();
        }
        ImGui::End();
      }
  }

  void Windows() {
    LogWindow(&ShowLogOutput, "#Log", LogData);
    LogWindow(&ShowLogstderr, "#Log stderr", stderrData);
    LogWindow(&ShowLogstdout, "#Log stdout", stdoutData);
  }

  void Init() {
    LogMan::Throw::InstallHandler(AssertHandler);
    LogMan::Msg::InstallHandler(MsgHandler);
  }

  void MenuItems() {
    ImGui::MenuItem("Log", nullptr, &ShowLogOutput);
    ImGui::MenuItem("LogO", nullptr, &ShowLogstdout);
    ImGui::MenuItem("LogE", nullptr, &ShowLogstderr);
  }
}

namespace Disasm {
  bool ShowCPUHostDisasm = true;
  bool ShowCPUGuestDisasm = true;

  uint64_t CurrentDisasmRIP {};
  std::string HostDisasmString;
  std::string GuestDisasmString;
  std::unique_ptr<FEX::Debugger::Disassembler> HostDisassembler;
  std::unique_ptr<FEX::Debugger::Disassembler> GuestDisassembler;

  void MenuItems() {
    ImGui::MenuItem("CPU Guest Disasm", nullptr, &ShowCPUGuestDisasm);
    ImGui::MenuItem("CPU Host Disasm", nullptr, &ShowCPUHostDisasm);
  }

  void Windows() {
    if (ImGui::Begin("#Guest Disasm", &ShowCPUGuestDisasm)) {
      ImGui::InputTextMultiline("", const_cast<char*>(GuestDisasmString.c_str()), GuestDisasmString.size(), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::End();

    if (ImGui::Begin("#Host Disasm", &ShowCPUHostDisasm)) {
      ImGui::InputTextMultiline("", const_cast<char*>(HostDisasmString.c_str()), HostDisasmString.size(), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::End();
  }

  uint8_t *GetPointerFromRegions(uint64_t PC) {
    for (auto &Region : MemoryViewer::MappedRegions) {
      if (Region.contains(PC)) {
        uint64_t Offset = PC - Region.Offset;
        return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(Region.Ptr) + Offset);
      }
    }

    return nullptr;
  }

  void DisasmGuest(uint64_t PC, int Size) {
    CurrentDisasmRIP = PC;
    uint8_t *GuestCode = GetPointerFromRegions(PC);
    uint32_t InstCount{};
    GuestDisasmString = GuestDisassembler->Disassemble(GuestCode, Size, 255, PC, &InstCount);

    bool DebugBlockExists;
    bool HostCodeBlockExists;
    uint8_t *HostCodePtr;
    FEXCore::Core::DebugData DebugData;
    HostCodeBlockExists = FEXCore::Context::Debug::FindHostCodeForRIP(FEX::DebuggerState::GetContext(), PC, &HostCodePtr);
    DebugBlockExists = FEXCore::Context::Debug::GetDebugDataForRIP(FEX::DebuggerState::GetContext(), PC, &DebugData);

    if (!DebugBlockExists || !HostCodeBlockExists || DebugData.HostCodeSize == 0) {
      // Didn't have a block backing this PC.
      HostDisasmString = "<No Backing>";
    }
    else {
      HostDisasmString = HostDisassembler->Disassemble(static_cast<uint8_t*>(HostCodePtr), DebugData.HostCodeSize, -1U, reinterpret_cast<uint64_t>(HostCodePtr), &InstCount);
    }
  }

  void Disasm(uint64_t PC) {
    CurrentDisasmRIP = PC;
    uint8_t *GuestCode = GetPointerFromRegions(PC);
    uint32_t InstCount{};

    bool DebugBlockExists;
    bool HostCodeBlockExists;
    uint8_t *HostCodePtr;
    FEXCore::Core::DebugData DebugData;
    HostCodeBlockExists = FEXCore::Context::Debug::FindHostCodeForRIP(FEX::DebuggerState::GetContext(), PC, &HostCodePtr);
    DebugBlockExists = FEXCore::Context::Debug::GetDebugDataForRIP(FEX::DebuggerState::GetContext(), PC, &DebugData);

    GuestDisasmString = GuestDisassembler->Disassemble(GuestCode, DebugData.GuestCodeSize, DebugData.GuestInstructionCount, PC, &InstCount);

    if (!DebugBlockExists || !HostCodeBlockExists || DebugData.HostCodeSize == 0) {
      // Didn't have a block backing this PC.
      HostDisasmString = "<No Backing>";
    }
    else {
      HostDisasmString = HostDisassembler->Disassemble(static_cast<uint8_t*>(HostCodePtr), DebugData.HostCodeSize, -1U, reinterpret_cast<uint64_t>(HostCodePtr), &InstCount);
    }
  }

  void Init() {
    HostDisassembler = FEX::Debugger::CreateHostDisassembler();
    GuestDisassembler = FEX::Debugger::CreateGuestDisassembler();
  }
}

namespace IR {
  bool ShowCPUIR = true;
  bool ShowCPUIRList = true;

  struct IRDebugData {
    FEXCore::Core::DebugData *Debug;
    uint64_t RIP;
    std::string RIPString;
    std::string GuestCodeSize;
    std::string GuestInstructionCount;
  };

  std::vector<IRDebugData> IRListTexts;
  int CPUIRListCurrentItem {};

  struct IRFileHeader {
    uint64_t RIP;
    size_t IRSize;
  };

  bool HadSelectedSaveIR{};
  ImGuiFs::Dialog Dialog;
  FEX::Debugger::IR::Lexer Lexer;
  std::vector<FEXImGui::IRLines> IRLines;

  //void CalculateCFGLines(FEXCore::IR::IntrusiveIRList *IR) {
  //  IRLines.clear();
  //  size_t Line {};
  //  size_t i {};
  //  size_t Size = IR->GetOffset();

  //  // From->To: Offset->AlignmentmentType
  //  std::unordered_map<uint64_t, uint32_t> MapFromTo{};

  //  // To->From: AlignmentType->Offset
  //  std::unordered_map<uint32_t, std::vector<uint64_t>> MapToFrom{};

  //  // Offset->Line
  //  std::unordered_map<uint32_t, uint64_t> OffsetToLine{};

  //  while (i != Size) {
  //    auto IROp = IR->GetOp(i);
  //    auto OpSize = FEXCore::IR::GetSize(IROp->Op);

  //    switch (IROp->Op) {
  //    case FEXCore::IR::OP_JUMP: {
  //      auto Op = IROp->C<FEXCore::IR::IROp_Jump>();
  //      MapFromTo[i] = Op->Location.Off;
  //      MapToFrom[Op->Location.Off].emplace_back(i);
  //    break;
  //    }
  //    case FEXCore::IR::OP_CONDJUMP: {
  //      auto Op = IROp->C<FEXCore::IR::IROp_CondJump>();
  //      MapFromTo[i] = Op->Location.Off;
  //      MapToFrom[Op->Location.Off].emplace_back(i);
  //    break;
  //    }

  //    default: break;
  //    }

  //    OffsetToLine[i] = Line;
  //    i += OpSize;
  //    ++Line;
  //  }

  //  for (auto From : MapFromTo) {
  //    auto To = MapToFrom.find(From.second);
  //    auto FromLine = OffsetToLine[From.first];
  //    auto ToLine = OffsetToLine[To->first];
  //    IRLines.emplace_back(FEXImGui::IRLines{FromLine, ToLine});
  //  }
  //}

  bool IRListGetter(void *data, int idx, const char** out_text) {
    int Type = reinterpret_cast<uintptr_t>(data);
    switch (Type) {
    case 1:
      *out_text = IRListTexts[idx].RIPString.c_str();
    break;
    case 2:
      *out_text = IRListTexts[idx].GuestCodeSize.c_str();
    break;
    case 3:
      *out_text = IRListTexts[idx].GuestInstructionCount.c_str();
    break;
    }
    return true;
  }

  void MenuItems() {
    ImGui::MenuItem("IR Viewer", nullptr, &ShowCPUIR);
    ImGui::MenuItem("IR List", nullptr, &ShowCPUIRList);
  }

  void Windows() {
    auto SortByRIP = [](IRDebugData const &a, IRDebugData const &b) -> bool {
        return a.RIP < b.RIP;
    };

    auto SortByCodeSize = [](IRDebugData const &a, IRDebugData const &b) -> bool {
        return a.Debug->GuestCodeSize < b.Debug->GuestCodeSize;
    };
    auto SortByInstCount = [](IRDebugData const &a, IRDebugData const &b) -> bool {
        return a.Debug->GuestInstructionCount < b.Debug->GuestInstructionCount;
    };

    if (ImGui::Begin("#CPU IR Entries", &ShowCPUIRList)) {
      if (ImGui::Button("Sort RIP")) {
        std::sort(IRListTexts.begin(), IRListTexts.end(), SortByRIP);
      }
      ImGui::SameLine();
      if (ImGui::Button("Sort CodeSize")) {
        std::sort(IRListTexts.begin(), IRListTexts.end(), SortByCodeSize);
      }

      ImGui::SameLine();
      if (ImGui::Button("Sort Inst Count")) {
        std::sort(IRListTexts.begin(), IRListTexts.end(), SortByInstCount);
      }

      ImGui::Columns(3);
      ImGui::Text("RIPS");
      ImGui::NextColumn();
      ImGui::Text("CodeSize");
      ImGui::NextColumn();
      ImGui::Text("InstructionCount");
      ImGui::NextColumn();

      ImGui::PushItemWidth(-1.0);

      bool UpdateSelection = false;
      UpdateSelection |= ImGui::ListBox("##RIP", &CPUIRListCurrentItem, IRListGetter, reinterpret_cast<void*>(1), IRListTexts.size());
      ImGui::PopItemWidth();

      ImGui::NextColumn();
      ImGui::PushItemWidth(-1.0);
      UpdateSelection |= ImGui::ListBox("##CodeSize", &CPUIRListCurrentItem, IRListGetter, reinterpret_cast<void*>(2), IRListTexts.size());
      ImGui::PopItemWidth();

      ImGui::NextColumn();
      ImGui::PushItemWidth(-1.0);
      UpdateSelection |= ImGui::ListBox("##InstCount", &CPUIRListCurrentItem, IRListGetter, reinterpret_cast<void*>(3), IRListTexts.size());
      ImGui::PopItemWidth();

      if (UpdateSelection) {
         uint64_t PC = IRListTexts[CPUIRListCurrentItem].RIP;
        std::stringstream out;
        FEX::DebuggerState::GetIR(&out, PC);
        Disasm::Disasm(PC);
        Logging::IRData = out.str();

        // FEXCore::IR::IntrusiveIRList *ir;
        // bool HadIR = FEXCore::Context::Debug::FindIRForRIP(FEX::DebuggerState::GetContext(), Disasm::CurrentDisasmRIP, &ir);
        // if (HadIR) {
        //   IR::CalculateCFGLines(ir);
        // }
      }
    }

    ImGui::End();

    if (ImGui::Begin("#IR Viewer", &ShowCPUIR)) {
      HadSelectedSaveIR = ImGui::Button("Save IR");
      FEXImGui::CustomIRViewer(Logging::IRData.c_str(), Logging::IRData.size(), &IRLines);
    }
    ImGui::End();

    char const *InitialPath;
    char const *File;

    InitialPath = Dialog.saveFileDialog(HadSelectedSaveIR, "./");
    File = Dialog.getChosenPath();
    if (strlen(InitialPath) > 0 && strlen(File) > 0) {
      // FEXCore::IR::IntrusiveIRList *ir;
      // bool HadIR = FEXCore::Context::Debug::FindIRForRIP(FEX::DebuggerState::GetContext(), Disasm::CurrentDisasmRIP, &ir);
      // if (HadIR) {
      //   IRFileHeader Header;
      //   Header.RIP = Disasm::CurrentDisasmRIP;
      //   Header.IRSize = ir->GetOffset();
      //   std::fstream IRFile;
      //   IRFile.open(File, std::fstream::out | std::fstream::binary);
      //   LogMan::Throw::A(IRFile.is_open(), "Failed to open file");

      //   IRFile.write(reinterpret_cast<char*>(&Header), sizeof(Header));
      //   IRFile.write(ir->GetOpAs<char>(0), Header.IRSize);
      //   IRFile.close();
      // }
    }
  }

  void LoadIR(char const *Filename) {
    IRFileHeader Header;
    std::fstream IRFile;
    IRFile.open(Filename, std::fstream::in | std::fstream::binary);
    LogMan::Throw::A(IRFile.is_open(), "Failed to open file");

    IRFile.read(reinterpret_cast<char*>(&Header), sizeof(Header));

    // FEXCore::IR::IntrusiveIRList IR(Header.IRSize);
    // char *Ptr = IR.SetupForLoad<char>(Header.IRSize);
    // IRFile.read(Ptr, Header.IRSize);
    // IRFile.close();
    // FEXCore::Context::Debug::SetIRForRIP(FEX::DebuggerState::GetContext(), Header.RIP, &IR);
    // FEX::DebuggerState::SetHasNewState();
    // FEX::DebuggerState::CallNewState();
  }

  void NewState() {
    CPUIRListCurrentItem = 0;
    IRListTexts.clear();

    // CPU core could have been removed
    if (!FEX::DebuggerState::ActiveCore()) {
      return;
    }

    FEXCore::Core::ThreadState *State = FEXCore::Context::Debug::GetThreadState(FEX::DebuggerState::GetContext());
    FEXCore::Core::InternalThreadState *TS = reinterpret_cast<FEXCore::Core::InternalThreadState*>(State);

    auto &IRList = TS->IRLists;
    auto &DebugData = TS->DebugData;

    for (auto &IR : IRList) {
       std::ostringstream out;
       out << "0x" << std::hex << IR.first;
       auto Data = DebugData.find(IR.first);
       IRDebugData DebugData;
       DebugData.Debug = &Data->second;
       DebugData.RIP = IR.first;
       DebugData.RIPString = out.str();
       DebugData.GuestCodeSize = std::to_string(Data->second.GuestCodeSize);
       DebugData.GuestInstructionCount = std::to_string(Data->second.GuestInstructionCount);
       IRListTexts.emplace_back(DebugData);
    }
  }
}

// History handling
namespace History {
  enum HistoryType {
    TYPE_ELF,
    TYPE_TEST_HARNESS,
    TYPE_IR,
    TYPE_X86,
  };
  std::list<std::pair<HistoryType, std::string>> HistoryItems;

  void Save() {
    // Save History
    char Buffer[512];
    char *Dest;
    Dest = json_objOpen(Buffer, nullptr);
    Dest = json_objOpen(Dest, "History");
    for (auto &it : HistoryItems) {
      Dest = json_int(Dest, it.second.c_str(), it.first);
    }
    Dest = json_objClose(Dest);
    Dest = json_objClose(Dest);
    json_end(Dest);

    std::fstream HistoryFile;
    HistoryFile.open("History.json", std::fstream::out);
    LogMan::Throw::A(HistoryFile.is_open(), "Failed to open file");

    HistoryFile.write(Buffer, strlen(Buffer));
    HistoryFile.close();
  }

  void Add(HistoryType Type, const char *Filename) {
    for (auto it = HistoryItems.begin(); it != HistoryItems.end(); ++it) {
      if (it->first == Type && it->second == Filename) {
        if (it != HistoryItems.begin()) {
          HistoryItems.splice(HistoryItems.begin(), HistoryItems, it, std::next(it));
          Save();
        }
        return;
      }
    }

    HistoryItems.emplace_front(std::make_pair(Type, Filename));
    if (HistoryItems.size() > 5) {
      auto it = HistoryItems.begin();
      std::advance(it, 5);
      HistoryItems.erase(it, HistoryItems.end());
    }
    Save();
  }

  void MenuItems() {
    for (auto &item : HistoryItems) {
      std::string Item{};
      switch (item.first) {
      case TYPE_ELF: Item = "ELF: "; break;
      case TYPE_TEST_HARNESS: Item = "Test: "; break;
      case TYPE_IR: Item = "IR: "; break;
      case TYPE_X86: Item = "X86: "; break;
      }
      auto Last = item.second.find_last_of('/') + 1;
      std::string Filename = item.second.substr(Last);
      Item += Filename;
      if (ImGui::MenuItem(Item.c_str(), nullptr, nullptr, !FEX::DebuggerState::ActiveCore() || item.first == TYPE_IR)) {
        if (item.first == TYPE_IR) {
          IR::LoadIR(item.second.c_str());
        }
        else {
          FEX::DebuggerState::Create(item.second.c_str(), item.first == TYPE_ELF);
        }
        History::Add(item.first, item.second.c_str());
      }
    }
  }

  void Init() {
    // Load History
    std::fstream HistoryFile;
    std::vector<char> Data;
    HistoryFile.open("History.json", std::fstream::in);
    if (HistoryFile.is_open()) {
      HistoryFile.seekg(0, std::fstream::end);
      size_t FileSize = HistoryFile.tellg();
      HistoryFile.seekg(0, std::fstream::beg);

      Data.resize(FileSize);
      HistoryFile.read(&Data.at(0), FileSize);
      HistoryFile.close();

      json_t elem[32];
      json_t const* json = json_create(&Data.at(0), elem, sizeof(elem) / sizeof(json_t));

      json_t const* HistoryList = json_getProperty(json, "History");
      for (json_t const* HistoryItem = json_getChild(HistoryList);
          HistoryItem != nullptr;
          HistoryItem = json_getSibling(HistoryItem)) {
          const char* HistoryName = json_getName(HistoryItem);
          const char* HistoryType = json_getValue(HistoryItem);
          History::Add(static_cast<History::HistoryType>(atol(HistoryType)), HistoryName);
      }
    }
  }
}

// Config
namespace Config {
  bool ShowConfig = true;
  struct Configs {
    FEX::Config::Value<uint64_t> ConfigMaxInst{"MaxInst", 255};
    FEX::Config::Value<uint8_t> ConfigCore{"Core", 0};
    FEX::Config::Value<uint8_t> ConfigRunningMode{"RunningMode", 0};
    FEX::Config::Value<bool> ConfigMultiblock{"Multiblock", false};
  };

  std::unique_ptr<Configs> Config;
  int NewInstMax {};
  bool Multiblock;

  void Window() {
    if (ImGui::Begin("#Config", &ShowConfig)) {
      if (ImGui::SliderInt("Max Inst", &NewInstMax, -1, 255)) {
        Config->ConfigMaxInst.Set(NewInstMax);
      }

      if (ImGui::Checkbox("Multiblock", &Multiblock)) {
        Config->ConfigMultiblock.Set(Multiblock);
      }
    }
    ImGui::End();
  }

  void Init() {
    Config = std::make_unique<Configs>();
    NewInstMax = Config->ConfigMaxInst();
    Multiblock = Config->ConfigMultiblock();
    FEX::DebuggerState::SetCoreType(static_cast<FEXCore::Config::ConfigCore>(Config::Config->ConfigCore()));
    FEX::DebuggerState::SetRunningMode(Config::Config->ConfigRunningMode());
  }

  void Shutdown() {
    Config::Config->ConfigCore.Set(FEX::DebuggerState::GetCoreType());
    Config::Config->ConfigRunningMode.Set(FEX::DebuggerState::GetRunningMode());
  }
}

// Memory viewer window
namespace MemoryViewer {
  bool ShowMemoryWindow = false;
  int MappedRegionsCurrentItem {};
  int MappedRegionsMemoryCurrentItem {};
  int CodeSize {128};
  uint64_t CurrentAddress {};
  char CurrentAddressString[32]{};
  bool MappedRegionGetter(void *data, int idx, const char** out_text) {
    auto &Region = MappedRegions[idx];
    static char Buf[512];
    snprintf(Buf, 512, "[0x%lx, 0x%lx) - 0x%lx", Region.Offset, Region.Offset + Region.Size, Region.Size);
    *out_text = Buf;
    return true;
  }

  bool MappedRegionMemoryGetter(void *data, int idx, const char** out_text) {
    if (MappedRegions.empty())
      return false;

    auto &Region = MappedRegions[MappedRegionsCurrentItem];
    static char Buf[512]{};
    char BufChars[17]{};
    uint8_t *Ptr = static_cast<uint8_t*>(Region.Ptr);
    Ptr += idx * 16;

    for (int i = 0; i < 16; i++) {
      if (isprint(Ptr[i]))
        BufChars[i] = Ptr[i];
      else
        BufChars[i] = '.';
    }

    snprintf(Buf, 512,
        "%4lX > %02x %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\t%s",
      Region.Offset + idx * 16,
      Ptr[0], Ptr[1], Ptr[2], Ptr[3], Ptr[4], Ptr[5], Ptr[6], Ptr[7],
      Ptr[8], Ptr[9], Ptr[10], Ptr[11], Ptr[12], Ptr[13], Ptr[14], Ptr[15],
      BufChars
      );
    *out_text = Buf;
    return true;
  }

  void Window() {
    auto DisplayAddress = [&](uint64_t RIP) {
      MappedRegionsCurrentItem = 0;
      // First we need to find the RIP in the mapped regions
      // Skip first entry because it is the full 64GB of vmem
      for (size_t i = 1; i < MappedRegions.size(); ++i) {
        if (MappedRegions[i].contains(RIP)) {
          MappedRegionsCurrentItem = i;
          break;
        }
      }
      // Now we need to set our hex view listbox to the correct line
      MappedRegionsMemoryCurrentItem = static_cast<int>(RIP - MappedRegions[MappedRegionsCurrentItem].Offset) / 16;

      // Now set the Current Address textbox and string to the correct address
      LogMan::Msg::D("RIP is 0x%lx", RIP);
      CurrentAddress = RIP;
      std::ostringstream out{};
      out << "0x" << std::hex << CurrentAddress;
      strncpy(CurrentAddressString, out.str().c_str(), 32);
    };

    if (ImGui::Begin("#Memory Viewer", &ShowMemoryWindow)) {
      if (FEX::DebuggerState::ActiveCore() && !FEX::DebuggerState::IsCoreRunning()) {
        FEXCore::Context::Debug::GetMemoryRegions(FEX::DebuggerState::GetContext(), &MappedRegions);
      }

      if (!MappedRegions.empty()) {
        ImGui::PushItemWidth(-1.0);
        if (ImGui::ListBox("Mapped Regions", &MappedRegionsCurrentItem, MappedRegionGetter, nullptr, MappedRegions.size())) {
        }
        ImGui::PopItemWidth();

        if (ImGui::InputText("RIP:", CurrentAddressString, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
          DisplayAddress(std::stoull(CurrentAddressString, nullptr, 0));
        }

        ImGui::DragInt("Size", &CodeSize, 1.0f, 1, 255);
        if (ImGui::Button("Disasm")) {
          CurrentAddress = std::stoull(CurrentAddressString, nullptr, 0);
          Disasm::DisasmGuest(CurrentAddress, CodeSize);
        }
        if (FEX::DebuggerState::IsCoreRunning()) {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("Compile RIP")) {
          FEXCore::Context::Debug::CompileRIP(FEX::DebuggerState::GetContext(), CurrentAddress);

          std::stringstream out;
          FEX::DebuggerState::GetIR(&out, CurrentAddress);
          Disasm::Disasm(CurrentAddress);
          Logging::IRData = out.str();

          // FEXCore::IR::IntrusiveIRList *ir;
          // bool HadIR = FEXCore::Context::Debug::FindIRForRIP(FEX::DebuggerState::GetContext(), CurrentAddress, &ir);
          // if (HadIR) {
          //   IR::CalculateCFGLines(ir);
          // }
        }
        if (FEX::DebuggerState::IsCoreRunning()) {
          ImGui::PopStyleVar();
          ImGui::PopItemFlag();
        }

        if (ImGui::Button("Go to current RIP")) {
          if (FEX::DebuggerState::ActiveCore()) {
            DisplayAddress(CPUState::CurrentState.rip);
          }
        }

        if (ImGui::BeginChildFrame(ImGui::GetID("Hex"), ImVec2(-1, -1))) {
          ImGui::PushItemWidth(-1.0);

          if (FEXImGui::ListBox("##Memory Hex",
              &MappedRegionsMemoryCurrentItem,
              MappedRegionMemoryGetter,
              nullptr,
              static_cast<int>(MappedRegions[MappedRegionsCurrentItem].Size) / 16)) {
            CurrentAddress = MappedRegions[MappedRegionsCurrentItem].Offset + MappedRegionsMemoryCurrentItem * 16;
            std::ostringstream out{};
            out << "0x" << std::hex << CurrentAddress;
            strncpy(CurrentAddressString, out.str().c_str(), 32);
          }
          ImGui::PopItemWidth();
        }
        ImGui::EndChildFrame();
      }

    }
    ImGui::End();
  }

  void Menu() {
    if (ImGui::BeginMenu("Memory")) {
      ImGui::MenuItem("Memory Viewer", nullptr, &ShowMemoryWindow);
      ImGui::EndMenu();
    }
  }
}

namespace FileDialogs {
  bool HadSelectedLaunchHarness = false;
  bool HadSelectedLaunchELF = false;
  bool HadSelectedIR = false;
  bool HadSelectedX86 = false;

  ImGuiFs::Dialog DialogELF{};
  ImGuiFs::Dialog DialogTest{};
  ImGuiFs::Dialog DialogIR{};
  ImGuiFs::Dialog DialogX86{};

  void MenuItems() {
    if (ImGui::BeginMenu("Launch")) {
      HadSelectedLaunchELF = ImGui::MenuItem("ELF", nullptr, false, !FEX::DebuggerState::ActiveCore());
      HadSelectedLaunchHarness = ImGui::MenuItem("TestHarness", nullptr, false, !FEX::DebuggerState::ActiveCore());
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Load")) {
      HadSelectedIR = ImGui::MenuItem("IR", nullptr, false, FEX::DebuggerState::ActiveCore());
      HadSelectedX86 = ImGui::MenuItem("X86", nullptr, false, !FEX::DebuggerState::ActiveCore());
      ImGui::EndMenu();
    }
    //ImGui::MenuItem("Connect");
  }

  void Windows() {
    char const *InitialPath;
    char const *File;

    InitialPath = DialogELF.chooseFileDialog(HadSelectedLaunchELF, "./");
    File = DialogELF.getChosenPath();
    if (strlen(InitialPath) > 0 && strlen(File) > 0) {
      FEX::DebuggerState::Create(File, true);
      History::Add(History::TYPE_ELF, File);
    }

    InitialPath = DialogTest.chooseFileDialog(HadSelectedLaunchHarness, "./");
    File = DialogTest.getChosenPath();
    if (strlen(InitialPath) > 0 && strlen(File) > 0) {
      FEX::DebuggerState::Create(File, false);
      History::Add(History::TYPE_TEST_HARNESS, File);
    }

    InitialPath = DialogIR.chooseFileDialog(HadSelectedIR, "./");
    File = DialogIR.getChosenPath();
    if (strlen(InitialPath) > 0 && strlen(File) > 0) {
      IR::LoadIR(File);
      History::Add(History::TYPE_IR, File);
    }

    InitialPath = DialogX86.chooseFileDialog(HadSelectedX86, "./");
    File = DialogX86.getChosenPath();
    if (strlen(InitialPath) > 0 && strlen(File) > 0) {
      FEX::DebuggerState::Create(File, false);
      History::Add(History::TYPE_X86, File);
    }

    HadSelectedLaunchHarness = false;
    HadSelectedLaunchELF = false;
    HadSelectedIR = false;
    HadSelectedX86 = false;
  }
}

void NewStateUpdate() {
  CPUState::NewState();
  IR::NewState();
}

void Init() {
  Logging::Init();

  FEX::DebuggerState::RegisterNewStateCallback(NewStateUpdate);

  Disasm::Init();
  Config::Init();
  History::Init();
}

void Shutdown() {
  FEX::DebuggerState::Close();
  Config::Shutdown();
  History::Save();
}

void DrawDebugUI(GLContext::Context *Context) {
  ImGuiIO& io = ImGui::GetIO();
  double current_time = glfwGetTime();
  io.DeltaTime = g_Time > 0.0 ? static_cast<float>(current_time - g_Time) : static_cast<float>(1.0f/60.0f);
  g_Time = current_time;

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

  bool CanStep = FEX::DebuggerState::ActiveCore();
  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("File")) {
      FileDialogs::MenuItems();
      if (ImGui::MenuItem("Close", nullptr, false, FEX::DebuggerState::ActiveCore())) {
        FEX::DebuggerState::Close();
      }
      ImGui::Separator();
      History::MenuItems();
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("CPU")) {
      ImGui::MenuItem("CPU Stats", nullptr, &CPUStats::ShowCPUStats);
      CPUState::MenuItems();
      ImGui::Separator();
      Disasm::MenuItems();
      ImGui::Separator();
      IR::MenuItems();
      ImGui::EndMenu();
    }

    MemoryViewer::Menu();

    if (ImGui::BeginMenu("Debug")) {
      if (ImGui::MenuItem("Step", "F11", false, CanStep)) {
        FEX::DebuggerState::Step();
      }
      if (ImGui::MenuItem("Pause", "Shift+F5", false, CanStep)) {
        FEX::DebuggerState::Pause();
      }
      if (ImGui::MenuItem("Continue", "F5", false, CanStep)) {
        FEX::DebuggerState::Continue();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Log")) {
      Logging::MenuItems();
      ImGui::EndMenu();
    }
    CPUState::Status();

    ImGui::EndMenuBar();
  }

  ImGui::End(); // End dockspace

  Disasm::Windows();
  CPUState::Windows();
  CPUStats::Window();
  Config::Window();
  IR::Windows();
  MemoryViewer::Window();
  FileDialogs::Windows();
  Logging::Windows();

  ImGui::Render();
  if (CanStep) {
    if (ImGui::IsKeyPressed(GLFW_KEY_F11)) {
      FEX::DebuggerState::Step();
    }

    if (ImGui::IsKeyPressed(GLFW_KEY_F5) && io.KeyShift) {
      FEX::DebuggerState::Pause();
    }

    if (ImGui::IsKeyPressed(GLFW_KEY_F5) && !io.KeyShift) {
      FEX::DebuggerState::Continue();
    }
  }
}

}
