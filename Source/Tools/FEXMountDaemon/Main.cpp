#include <atomic>
#include <bits/types/siginfo_t.h>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <limits.h>
#include <mutex>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <thread>

namespace {

void SignalShutdown();
namespace EPollWatcher {
  static int epoll_fd{};
  static std::thread EPollThread{};
  std::atomic<uint64_t> NumPipesWatched{};
  std::atomic<bool> EPollWatcherShutdown {false};
  std::chrono::time_point<std::chrono::system_clock> TimeWhileZeroFDs{};
  // Timeout is ten seconds
  constexpr std::chrono::duration<size_t> TimeoutPeriod = std::chrono::seconds(10);

  uint64_t NumPipesRemaining() {
    return NumPipesWatched.load();
  }

  void AddPipeToWatch(int pipe) {
    struct epoll_event evt{};
    evt.events = EPOLLERR; // This event will return when the read end of a pipe is closed
    evt.data.fd = pipe; // Just return the pipe in the user data
    int Result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe, &evt);
    if (Result == -1) {
      fprintf(stderr, "[FEXMountDaemon] epoll_ctl returned error %d %s\n", errno, strerror(errno));
    }
    else {
      ++NumPipesWatched;
    }
  }

  void RemovePipeToWatch(int pipe) {
    int Result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pipe, nullptr);
    if (Result == -1) {
      fprintf(stderr, "[FEXMountDaemon] epoll_ctl returned error %d %s\n", errno, strerror(errno));
    }
    else {
      --NumPipesWatched;
    }

    // If the number of pipes we are watching drops to zero then start a timer so we can exit
    if (NumPipesRemaining() == 0) {
      TimeWhileZeroFDs = std::chrono::system_clock::now();
    }
  }

  void EPollWatch() {
    constexpr size_t MAX_EVENTS = 16;
    struct epoll_event Events[MAX_EVENTS]{};
    while (!EPollWatcherShutdown.load()) {
      // Loop every ten seconds
      // epoll_pwait2 only available since kernel 5.11...
      int Result = epoll_pwait(epoll_fd, Events, MAX_EVENTS, 10 * 1000, nullptr);
      if (Result == -1) {
        // EINTR is common here
      }
      else {
        for (size_t i = 0; i < Result; ++i) {
          auto &Event = Events[i];
          if (Event.events & EPOLLERR) {
            // This pipe's read end has closed
            // No need to watch it anymore
            RemovePipeToWatch(Event.data.fd);
          }
        }

        // If we are at zero pipes then check our timer if we are past the timeout period to exit
        if (NumPipesRemaining() == 0) {
          auto Now = std::chrono::system_clock::now();
          auto Dur = Now - TimeWhileZeroFDs;
          if (Dur >= TimeoutPeriod) {
            // We need to shutdown now
            ::SignalShutdown();
          }
        }
      }
    }
  }

  void SetupEPoll() {
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    EPollThread = std::thread{EPollWatcher::EPollWatch};
  }

  void SignalShutdown() {
    EPollWatcherShutdown = true;
  }

  void ShutdownEPoll() {
    SignalShutdown();
    close(epoll_fd);
    EPollThread.join();
  }
}

namespace SocketWatcher {
  static int socket_fd{};
  static std::thread SocketThread{};
  std::atomic<bool> SocketShutdown {false};

  void SocketFunction() {
    while (!SocketShutdown.load()) {
      // Wait for data coming in
      struct pollfd pfd{};
      pfd.fd = socket_fd;
      pfd.events = POLLIN;

      // Wait for ten seconds
      struct timespec ts{};
      ts.tv_sec = 10;

      int Result = ppoll(&pfd, 1, &ts, nullptr);
      if (Result == -1) {
        // EINTR is common here
      }
      else if (Result > 0) {
        // We got data to grab
        struct msghdr msg{};
        struct iovec iov{};
        char iov_data{};

        // Setup the ancillary buffer. This is where we will be getting pipe FDs
        // We only need 4 bytes for the FD
        constexpr size_t CMSG_SIZE = CMSG_SPACE(sizeof(int));
        union AncillaryBuffer {
          struct cmsghdr Header;
          uint8_t Buffer[CMSG_SIZE];
        };
        AncillaryBuffer AncBuf{};

        // Set up message header
        msg.msg_name = nullptr;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        // Setup iov. We won't be receiving any real data here
        iov.iov_base = &iov_data;
        iov.iov_len = sizeof(iov_data);

        // Now link to our ancilllary buffer
        msg.msg_control = AncBuf.Buffer;
        msg.msg_controllen = CMSG_SIZE;

        ssize_t DataResult = recvmsg(socket_fd, &msg, 0);
        if (DataResult == -1) {
          // Sometimes get a spurious read?
        }
        else {
          // Now that we have the data, we can extract the FD from the ancillary buffer
          struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

          // Do some error checking
          if (cmsg == nullptr ||
              cmsg->cmsg_len != CMSG_LEN(sizeof(int)) ||
              cmsg->cmsg_level != SOL_SOCKET ||
              cmsg->cmsg_type != SCM_RIGHTS) {
            fprintf(stderr, "[FEXMountDaemon:SocketWatcher] cmsg data was incorrect\n");
          }
          else {
            // Now that we know the cmsg is sane, read the FD
            int NewFD{};
            memcpy(&NewFD, CMSG_DATA(cmsg), sizeof(NewFD));

            // Add to to the epoll watcher
            EPollWatcher::AddPipeToWatch(NewFD);
          }
        }
      }
    }
  }

  bool CreateServerSocket(std::string &SocketPath) {
    // Unlink the socket file if it exists
    // We are being asked to create a daemon, not error check
    // We don't care if this failed or not
    remove(SocketPath.c_str());

    // Create the initial unix socket
    socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
      fprintf(stderr, "[FEXMountDaemon:SocketWatcher] Couldn't create AF_UNIX socket: %d %s\n", errno, strerror(errno));
      return false;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SocketPath.data(), sizeof(addr.sun_path));

    // Bind the socket to the path
    int Result = bind(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (Result == -1) {
      fprintf(stderr, "[FEXMountDaemon:SocketWatcher] Couldn't bind AF_UNIX socket: %d %s\n", errno, strerror(errno));
      return false;
    }

    return true;
  }

  bool SetupSocketWatcher(std::string &SocketPath) {
    if (!CreateServerSocket(SocketPath)) {
      return false;
    }

    SocketThread = std::thread{SocketWatcher::SocketFunction};
    return true;
  }

  void SignalShutdown() {
    SocketShutdown = true;
  }

  void ShutdownSocketWatcher(std::string &SocketPath) {
    SignalShutdown();
    close(socket_fd);
    SocketThread.join();

    remove(SocketPath.c_str());
  }
}

namespace INotifyWatcher {
  static int lock_fd {-1};
  static std::condition_variable WaitCV{};
  static std::mutex WaitMutex{};
  static std::atomic<bool> INotifyShutdown {false};
  enum LockFailure {
    LOCK_FAIL_FATAL,
    LOCK_FAIL_EXISTS,
    LOCK_FAIL_CREATION_RACE,
    LOCK_FAIL_CREATED,
  };

  constexpr int USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;

  LockFailure CreateINotifyLock(std::string LockPath, const char *MountPath) {
    lock_fd = open(LockPath.c_str(), O_RDONLY, USER_PERMS);
    if (lock_fd != -1) {
      // LockFD already existed!
      // This will have now refcounted the existing daemon!
      close(lock_fd);
      return LOCK_FAIL_EXISTS;
    }

    lock_fd = open(LockPath.c_str(), O_CREAT | O_RDWR, USER_PERMS);
    if (lock_fd == -1) {
      // Couldn't open lock file for some reason
      // Likely read only file system
      return LOCK_FAIL_FATAL;
    }

    LockFailure Failure = LOCK_FAIL_FATAL;

    // Set up the write lock to ensure this doesn't race
    {
      // Attempt to open a write lease on the lock file
      // First thing, mask the signal from the least interface
      // By default it is SIGIO
      {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGIO);
        if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1) {
          goto err;
        }
      }

      // Set the file's lease signal
      // Even if we set it to default, this is necessary
      {
        int Res = fcntl(lock_fd, F_SETSIG, SIGIO);
        if (Res == -1) {
          // Shouldn't fail
          goto err;
        }
      }

      // Now attempt to get a write lock on this file
      {
        int Res = fcntl(lock_fd, F_SETLEASE, F_WRLCK);
        if (Res == -1) {
          // Couldn't get a write lock
          // This means another FEXMountDaemon is in the process of setting up a rootfs
          // Early exit, this will be mounted in another process
          Failure = LOCK_FAIL_CREATION_RACE;
          goto err;
        }
      }

      // Now that we have a lock on the file.
      // Write where we are going to be mounting
      // Nothing else can currently open the file for reads yet to see this
      write(lock_fd, MountPath, strlen(MountPath));
    }

    return LOCK_FAIL_CREATED;
    err:
    if (lock_fd != -1) {
      close (lock_fd);
    }

    return Failure;
  }

  void WatchLock() {
    // Let go of the file lease file to allow FEX to continue
    fcntl(lock_fd, F_SETLEASE, F_UNLCK);

    // Wait in a mutex to shutdown
    std::unique_lock<std::mutex> lk (WaitMutex);
    WaitCV.wait(lk, [] { return INotifyShutdown.load(); });
  }

  void RemoveLock(std::string LockPath) {
    // Remove the lock file itself
    unlink(LockPath.c_str());

    // Clear the lease on it since we are shutting down
    fcntl(lock_fd, F_SETLEASE, F_UNLCK);

    // Close the lock fd
    close(lock_fd);
  }

  void SignalShutdown() {
    INotifyShutdown = true;
    WaitCV.notify_all();
  }
}

void ActionHandler(int sig, siginfo_t *info, void *context) {
  if (sig == SIGCHLD) {
    if (EPollWatcher::NumPipesRemaining() != 0) {
      // Check the pipes we are watching to see if any exist
      // If our child process shutdown while our parent is still running
      // Then the parent loses its rootfs and problems occur
      fprintf(stderr, "FEXMountDaemon child process from squashfuse has closed while FEX instances still exist\n");
      fprintf(stderr, "Expect errors!\n");
    }
  }
  else {
    // Signal sent directly to process
    // Force a shutdown
    fprintf(stderr, "We are being told to shutdown with SIGTERM\n");
    fprintf(stderr, "Watch out! You might get dangling mount points!\n");
    ::SignalShutdown();
  }
}


void SignalShutdown() {
  INotifyWatcher::SignalShutdown();
  SocketWatcher::SignalShutdown();
  EPollWatcher::SignalShutdown();
}

}

int main(int argc, char **argv, char **envp) {
  if (argc < 4) {
    fprintf(stderr, "usage: %s <SquashFS> <MountPoint> <pipe wr>\n", argv[0]);
    return -1;
  }

  pid_t pid = fork();
  if (pid != 0) {
    // Parent is leaving to force this process to deparent itself
    // This lets this process become the child of whatever the reaper parent is
    // Do this up front so a FEXInterptreter instance doesn't erroneously pick up a SIGCHLD
    return 0;
  }

  const char *SquashFSPath = argv[1];
  const char *MountPath = argv[2];
  int pipe_wr = std::atoi(argv[3]);

  // Start the epoll watcher
  EPollWatcher::SetupEPoll();

  // Switch this process over to a new session id
  // Probably not required but allows this to become the process group leader of its session
  ::setsid();

  // Create local FDs so our internal forks can communicate
  int localfds[2];
  pipe2(localfds, 0);

  ::prctl(PR_SET_CHILD_SUBREAPER, 1);

  struct utsname uts{};
  uname (&uts);

  std::string LockPath = "/tmp/.FEX-";
  LockPath += std::filesystem::path(SquashFSPath).filename();
  LockPath += ".lock.";
  LockPath += uts.nodename;

  std::string SocketPath = "/tmp/.FEX-";
  SocketPath += std::filesystem::path(SquashFSPath).filename();
  SocketPath += ".socket.";
  SocketPath += uts.nodename;

  if (!SocketWatcher::SetupSocketWatcher(SocketPath)) {
    fprintf(stderr, "[FEXMountDaemon] Failed to setup socket watcher\n");
    return -1;
  }

  // Use lock files to ensure we aren't racing to mount multiple FSes
  auto Failure = INotifyWatcher::CreateINotifyLock(LockPath, MountPath);
  if (Failure == INotifyWatcher::LOCK_FAIL_FATAL) {
    fprintf(stderr, "[FEXMountDaemon] Failed to setup inotify lock\n");
    return -1;
  }

  if (Failure == INotifyWatcher::LOCK_FAIL_EXISTS ||
      Failure == INotifyWatcher::LOCK_FAIL_CREATION_RACE) {
    // If the lock already exists
    // Then we don't need to spin up the mounts at all
    // Cleanly exit early and let FEX know it can continue
    uint64_t c = 0;
    write(pipe_wr, &c, sizeof(c));
    return 0;
  }

  pid = fork();
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

    // SIGCHLD if squashfuse exits early
    sigaction(SIGCHLD, &act, nullptr);
    // SIGTERM if something is trying to terminate us
    sigaction(SIGTERM, &act, nullptr);
    // Ignore SIGPIPE, we will be checking for pipe closure which could send this signal
    signal(SIGPIPE, SIG_IGN);

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
    // FEX will stall on opening the lock file until we let go of the lease
    uint64_t c = 0;
    write(pipe_wr, &c, sizeof(c));

    // Watch our lock file now for users
    INotifyWatcher::WatchLock();

    INotifyWatcher::RemoveLock(LockPath);

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

  EPollWatcher::ShutdownEPoll();
  SocketWatcher::ShutdownSocketWatcher(SocketPath);

  return 0;
}
