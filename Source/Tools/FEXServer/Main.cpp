// SPDX-License-Identifier: MIT
#include "ArgumentLoader.h"
#include "Logger.h"
#include "PipeScanner.h"
#include "PortabilityInfo.h"
#include "ProcessPipe.h"
#include "SquashFS.h"
#include "Common/ArgumentLoader.h"
#include "Common/Config.h"
#include "Common/FEXServerClient.h"

#include <fmt/color.h>

#include <chrono>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <iterator>
#include <mutex>
#include <optional>
#include <poll.h>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

static timespec StartTime {};

// Set an empty style to disable coloring when FEXServer output is e.g. piped to a file
static std::optional<fmt::text_style> DisableColors = isatty(STDOUT_FILENO) ? std::nullopt : std::optional {fmt::text_style {}};

namespace Logging {
void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  const auto Output = fmt::format("{} {}\n", fmt::styled(LogMan::DebugLevelStr(Level), DisableColors.value_or(DebugLevelStyle(Level))), Message);
  write(STDOUT_FILENO, Output.c_str(), Output.size());
}

void AssertHandler(const char* Message) {
  return MsgHandler(LogMan::ASSERT, Message);
}

void ClientMsgHandler(int FD, FEXServerClient::Logging::PacketMsg* const Msg, const char* MsgStr) {
  if (!StartTime.tv_sec && !StartTime.tv_nsec) {
    StartTime = Msg->Header.Timestamp;
  }
  auto seconds = Msg->Header.Timestamp.tv_sec - StartTime.tv_sec - (Msg->Header.Timestamp.tv_nsec < StartTime.tv_nsec);
  auto nanos = (1'000'000'000 + Msg->Header.Timestamp.tv_nsec - StartTime.tv_nsec) % 1'000'000'000;
  char Metadata[128];
  auto Cursor =
    fmt::format_to(&Metadata[0], DisableColors.value_or(LogMan::DebugLevelStyle(Msg->Level)), "{}", LogMan::DebugLevelStr(Msg->Level));
  Cursor = fmt::format_to(Cursor, DisableColors.value_or(fmt::fg(fmt::color::light_gray)), " {}|{} ", Msg->Header.PID, Msg->Header.TID);
  Cursor = fmt::format_to(Cursor, DisableColors.value_or(fmt::fg(fmt::color::gray)), "{}.{:03}", seconds, nanos / 1000000);
  *Cursor = 0;
  auto Output = fmt::format("{} {}\n", Metadata, MsgStr);
  write(STDERR_FILENO, Output.c_str(), Output.size());
}
} // namespace Logging

namespace {
void ActionHandler(int sig, siginfo_t* info, void* context) {
  // TODO: Fix this
  if (sig == SIGINT) {
    // Someone trying to kill us. Shutdown.
    ProcessPipe::Shutdown();

    // Clear "^C" string that most terminals print when pressing Ctrl+C.
    fprintf(stderr, "\r");
    return;
  }
  _exit(1);
}

void ActionIgnore(int sig, siginfo_t* info, void* context) {}

void SetupSignals() {
  // Setup our signal handlers now so we can capture some events
  struct sigaction act {};
  act.sa_sigaction = ActionHandler;
  act.sa_flags = SA_SIGINFO;

  // SIGTERM if something is trying to terminate us
  sigaction(SIGTERM, &act, nullptr);
  // SIGINT if something is trying to terminate us
  sigaction(SIGINT, &act, nullptr);

  // SIGUSR1 just to interrupt syscalls
  act.sa_sigaction = ActionIgnore;
  sigaction(SIGUSR1, &act, nullptr);

  // Ignore SIGPIPE, we will be checking for pipe closure which could send this signal
  signal(SIGPIPE, SIG_IGN);
  // Reset SIGCHLD which is likely SIG_IGN if FEXInterpreter started the server.
  // We now wait for child processes with waitpid, newer libfuse also requires SIGCHLD to not be ignored by child processes.
  signal(SIGCHLD, SIG_DFL);
}

/**
 * @brief Deparents itself by forking and terminating the parent process.
 */
void DeparentSelf() {
  auto SystemdEnv = getenv("INVOCATION_ID");
  if (SystemdEnv) {
    // If FEXServer was launched through systemd then don't deparent, otherwise systemd kills the entire server.
    return;
  }

  pid_t pid = fork();

  if (pid != 0) {
    // Parent is leaving to force this process to deparent itself
    // This lets this process become the child of whatever the reaper parent is
    _exit(0);
  }
}
} // namespace

int main(int argc, char** argv, char** const envp) {
  auto Options = FEXServer::Config::Load(argc, argv);

  SetupSignals();

  if (Options.Foreground) {
    LogMan::Throw::InstallHandler(Logging::AssertHandler);
    LogMan::Msg::InstallHandler(Logging::MsgHandler);
  }

  if (!Options.Foreground) {
    DeparentSelf();
  }

  FEX::Config::LoadConfig({}, {}, envp, FEX::ReadPortabilityInformation());

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();

  if (Options.Wait) {
    int ServerPipe = FEXServerClient::ConnectToServer();
    if (ServerPipe != -1) {
      int FEXServerPID = FEXServerClient::RequestPIDFD(ServerPipe);
      close(ServerPipe);
      if (FEXServerPID != -1) {
        LogMan::Msg::IFmt("[FEXServer] Waiting for FEXServer to close");
        // We can't use waitid (P_PIDFD) here because the active FEXServer isn't a child of this process.
        // Use poll instead which will return once the pidfd closes.
        pollfd PollFD;
        PollFD.fd = FEXServerPID;
        PollFD.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;

        // Wait for a result on the pipe that isn't EINTR
        while (poll(&PollFD, 1, -1) == -1 && errno == EINTR)
          ;

        LogMan::Msg::IFmt("[FEXServer] FEXServer shutdown");
      }
      PipeScanner::ClosePipes();
    }
    return 0;
  }

  if (Options.Kill) {
    int ServerPipe = FEXServerClient::ConnectToServer();

    if (ServerPipe != -1) {
      FEXServerClient::RequestServerKill(ServerPipe);
      LogMan::Msg::DFmt("[FEXServer] Sent kill packet");
      PipeScanner::ClosePipes();
    }
    return 0;
  }

  if (!ProcessPipe::InitializeServerPipe()) {
    // Someone else already owns the FEXServer pipe
    PipeScanner::ClosePipes();
    return -1;
  }

  if (!ProcessPipe::InitializeServerSocket(true)) {
    // Couldn't create server socket for some reason
    PipeScanner::ClosePipes();
    return -1;
  }

  if (!ProcessPipe::InitializeServerSocket(false)) {
    // Couldn't create server socket for some reason
    PipeScanner::ClosePipes();
    return -1;
  }

  // Switch this process over to a new session id
  // Probably not required but allows this to become the process group leader of its session
  ::setsid();

  // Set process as a subreaper so subprocesses can't escape
  if (::prctl(PR_SET_CHILD_SUBREAPER, 1) == -1) [[unlikely]] {
    // If subreaper failed then squashfuse/erofsfuse can escape, which isn't fatal.
    LogMan::Msg::DFmt("[FEXServer] Couldn't set subreaper.");
  }

  if (Options.Foreground) {
    // Only start a log thread if we are in the foreground.
    // Prevents FEXInterpreter from trying to log to nothing.
    Logger::StartLogThread();
  }

  if (!SquashFS::InitializeSquashFS()) {
    LogMan::Msg::DFmt("[FEXServer] Couldn't mount squashfs");
    return -1;
  }

  // Close the pipes we found at the start
  // This will let FEXInterpreter know we are ready
  PipeScanner::ClosePipes();

  ProcessPipe::SetConfiguration(Options.Foreground, Options.PersistentTimeout ?: 1);

  // Actually spin up the request thread.
  // Any applications that were waiting for the socket to accept will then go through here.
  ProcessPipe::WaitForRequests();

  SquashFS::UnmountRootFS();

  Logger::Shutdown();

  return 0;
}
