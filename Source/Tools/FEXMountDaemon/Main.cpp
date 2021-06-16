#include <atomic>
#include <cstdlib>
#include <cstdint>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <linux/limits.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/inotify.h>

namespace {
static std::atomic<bool> ShuttingDown{};
static int ParentPIDProcess{};

void ActionHandler(int sig, siginfo_t *info, void *context) {
  if (sig == SIGUSR1 &&
      info->si_pid == ParentPIDProcess) {
    // Begin shutdown sequence
    ShuttingDown = true;
  }
  else if (sig == SIGCHLD) {
    if (!ShuttingDown.load()) {
      // If our child process shutdown while our parent is still running
      // Then the parent loses its rootfs and problems occur
      fprintf(stderr, "FEXMountDaemon child process from squashfuse has closed\n");
      fprintf(stderr, "Expect errors!\n");
      ShuttingDown = true;
    }
  }
  else {
    // Signal sent directly to process
    // Ignore it
  }
}
}

int main(int argc, char **argv, char **envp) {
  if (argc < 5) {
    fprintf(stderr, "usage: %s <SquashFS> <MountPoint> <Parent PID> <pipe wr>\n", argv[0]);
    return -1;
  }

  const char *SquashFSPath = argv[1];
  const char *MountPath = argv[2];
  ParentPIDProcess = std::atoi(argv[3]);
  int pipe_wr = std::atoi(argv[4]);

  // Switch this process over to a new session id
  // Probably not required but allows this to become the process group leader of its session
  ::setsid();

  // Create local FDs so our internal forks can communicate
  int localfds[2];
  pipe2(localfds, 0);

  ::prctl(PR_SET_PDEATHSIG, SIGUSR1);
  ::prctl(PR_SET_CHILD_SUBREAPER, 1);

  pid_t pid = fork();
  if (pid == 0) {
    // Child
    close(localfds[0]); // Close read side
    const char *argv[4];
    argv[0] = "/usr/bin/squashfuse";
    argv[1] = SquashFSPath;
    argv[2] = MountPath;
    argv[3] = nullptr;

    // Try and execute squashfuse to mount our rootfs
    if (execve(argv[0], (char * const*)argv, envp) == -1) {
      // Let the parent know that we couldn't execute for some reason
      uint64_t error{1};
      write(localfds[1], &error, sizeof(error));

      // Give a hopefully helpful error message for users
      fprintf(stderr, "'%s' Couldn't execute for some reason: %d %s\n", argv[0], errno, strerror(errno));
      fprintf(stderr, "To mount squashfs rootfs files you need squashfuse installed\n");
      fprintf(stderr, "Check your FUSE setup.\n");

      // End the child
      exit(1);
    }
  }
  else {
    // Parent

    close(localfds[1]); // Close the write side
    // Wait for the child to exit
    // This will happen with execve of squashmount or exit on failure
    waitpid(pid, nullptr, 0);

    // Setup our signal handlers now so we can capture some events
    struct sigaction act{};
    act.sa_sigaction = ActionHandler;
    act.sa_flags = SA_SIGINFO;

    // SIGUSR1 when FEX exits
    sigaction(SIGUSR1, &act, nullptr);
    // SIGCHLD if squashfuse exits early
    sigaction(SIGCHLD, &act, nullptr);
    // SIGTERM if something is trying to terminate us
    sigaction(SIGTERM, &act, nullptr);

    // Check the child pipe for messages
    pollfd PollFD;
    PollFD.fd = localfds[0];
    PollFD.events = POLLIN;

    int Result = poll(&PollFD, 1, 0);

    if (Result == 1 && PollFD.revents & POLLIN) {
      // Child couldn't execve for whatever reason
      // Remove the mount path and leave Just in case it was created
      rmdir(MountPath);

      // Tell FEX that we encountered an issue
      uint64_t c = 1;
      write(pipe_wr, &c, sizeof(c));
      return 1;
    }

    // Close the pipe now
    close(localfds[0]);

    // Tell FEX that it can continue booting
    uint64_t c = 0;
    write(pipe_wr, &c, sizeof(c));

    // Sleep until we receive a shutdown signal
    // Needs to loop in case of EINTR
    while (!ShuttingDown.load()) {
      select(0, nullptr, nullptr, nullptr, nullptr);
    }

    // fusermount for unmounting the mountpoint, then the squashfuse will exit automatically
    pid = fork();

    if (pid == 0) {
      const char *argv[5];
      argv[0] = "/bin/fusermount";
      argv[1] = "-u";
      argv[2] = "-q";
      argv[3] = MountPath;
      argv[4] = nullptr;

      if (execve(argv[0], (char * const*)argv, envp) == -1) {
        // Try another location
        argv[0] = "/usr/bin/fusermount";
        if (execve(argv[0], (char * const*)argv, envp) == -1) {
          fprintf(stderr, "fusermount failed to execute. You may have an mount living at '%s' to clean up now\n", SquashFSPath);
          fprintf(stderr, "Try `%s %s %s %s`\n", argv[0], argv[1], argv[2], argv[3]);
          exit(1);
        }
      }
    }
    else {
      // Wait for fusermount to leave
      waitpid(pid, nullptr, 0);

      // Remove the mount path and leave
      rmdir(MountPath);
    }
  }

  return 0;
}
