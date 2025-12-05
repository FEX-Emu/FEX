// SPDX-License-Identifier: MIT
#include "PortabilityInfo.h"
#include "Common/FEXServerClient.h"

#include <cstdio>
#include <unistd.h>
#include <poll.h>

void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  const auto Style = fmt::text_style {};
  const auto Output = fextl::fmt::format("{} {}\n", fmt::styled(LogMan::DebugLevelStr(Level), Style), Message);
  write(STDERR_FILENO, Output.c_str(), Output.size());
  fsync(STDERR_FILENO);
}

void AssertHandler(const char* Message) {
  return MsgHandler(LogMan::ASSERT, Message);
}

void SignalPVToContinue() {
  // Tell pressure-vessel that the startup was a success.
  const auto ReadyMsg = "READY=1\n";
  write(STDOUT_FILENO, ReadyMsg, strlen(ReadyMsg));

  // pressure-vessel is waiting for EOF on STDOUT from this process to ensure it can run FEX processes.
  // dup2 atomically replaces stdout with a copy of stderr to achieve this.
  dup2(STDERR_FILENO, STDOUT_FILENO);
}

struct PipesType {
  int read_pipe {-1};
  int write_pipe {-1};
};

PipesType get_pipe() {
  PipesType pipes {};
  pipe(&pipes.read_pipe);
  return pipes;
}

int main(int argc, const char** argv, char** const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  const auto PortableInfo = FEX::ReadPortabilityInformation();
  FEX::Config::LoadConfig({}, envp, PortableInfo);

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();

  auto pipes = get_pipe();

  // Set the write side to close on exec.
  fcntl(pipes.write_pipe, F_SETFD, FD_CLOEXEC);

  // Give the read end of the pipe to FEXServer.
  auto ServerFD = FEXServerClient::StartServer(PortableInfo.InterpreterPath, pipes.read_pipe);

  if (ServerFD == -1) {
    perror("Couldn't start FEXServer");
    return 126;
  }

  // FEXServer is now running. Tell PV to continue.
  SignalPVToContinue();

  // Don't need the read pipe anymore.
  close(pipes.read_pipe);
  pipes.read_pipe = -1;

  // Now that the server is started and watching our pipe, we can close the returned FD, as it'll stay open as long as the pipe is open.
  close(ServerFD);
  ServerFD = -1;

  // stdin will be a pipe, so wait until that FD is closed.
  while (true) {
    pollfd p {
      .fd = STDIN_FILENO,
      .events = POLLRDHUP,
      .revents = 0,
    };

    int events = poll(&p, 1, -1);
    if (events == -1 && errno == EINTR) {
      continue;
    }

    if (events > 0 && (p.revents & (POLLRDHUP | POLLERR | POLLHUP | POLLNVAL))) {
      // Error or pressure-vessel hung-up.
      break;
    }
  }

  // Terminating will clean-up.
  return 0;
}
