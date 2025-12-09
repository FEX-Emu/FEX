// SPDX-License-Identifier: MIT
#include "PortabilityInfo.h"
#include "Common/FEXServerClient.h"

#include <cstdio>
#include <errno.h>
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

void SignalPVToContinue(int* original_stdout) {
  // Tell pressure-vessel that the startup was a success.
  const auto ReadyMsg = "READY=1\n";
  write(*original_stdout, ReadyMsg, strlen(ReadyMsg));

  // pressure-vessel is waiting for EOF on STDOUT from this process to ensure it can run FEX processes.
  close(*original_stdout);
  *original_stdout = -1;
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

  // Move the ready-indicator pipe from stdout to some other fd,
  // and mark it so the FEXServer won't inherit it. Otherwise the FEXServer
  // will hold it open, preventing pressure-vessel from detecting that
  // we are ready.
  int original_stdout = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, /* minimum fd = */ 3);
  if (original_stdout < 0) {
    perror("F_DUPFD_CLOEXEC");
    return 126;
  }
  // Replace stdout with a copy of our original stderr.
  if (dup2(STDERR_FILENO, STDOUT_FILENO) != STDOUT_FILENO) {
    perror("dup2");
    return 126;
  }

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
  SignalPVToContinue(&original_stdout);

  // Don't need the read pipe anymore.
  close(pipes.read_pipe);
  pipes.read_pipe = -1;

  // Now that the server is started and watching our pipe, we can close the returned FD, as it'll stay open as long as the pipe is open.
  close(ServerFD);
  ServerFD = -1;

  // Do a blocking read, discarding any written data and wait for EOF.
  while (true) {
    char buf[4096];
    auto read_len = ::read(STDIN_FILENO, buf, sizeof(buf));
    if (read_len < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        // Interrupted, try again.
        continue;
      } else {
        // Error on read.
        break;
      }
    } else if (read_len == 0) {
      // EOF
      break;
    }
  }

  // Terminating will clean-up.
  return 0;
}
