#include <atomic>
#include <bits/types/siginfo_t.h>
#include <cstdlib>
#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/utsname.h>

namespace {
static std::atomic<bool> ForceShutdown{};
static std::atomic<bool> ParentShuttingDown{};
static int ParentPIDProcess{};

void ActionHandler(int sig, siginfo_t *info, void *context) {
  if (sig == SIGUSR1 &&
      info->si_pid == ParentPIDProcess) {
    // Begin shutdown sequence
    ParentShuttingDown = true;
  }
  else if (sig == SIGCHLD) {
    if (!ParentShuttingDown.load()) {
      // If our child process shutdown while our parent is still running
      // Then the parent loses its rootfs and problems occur
      fprintf(stderr, "FEXMountDaemon child process from squashfuse has closed\n");
      fprintf(stderr, "Expect errors!\n");
      ParentShuttingDown = true;
    }
  }
  else {
    // Signal sent directly to process
    // Force a shutdown
    fprintf(stderr, "We are being told to shutdown with SIGTERM\n");
    fprintf(stderr, "Watch out! You might get dangling mount points!\n");
    ForceShutdown = true;
  }
}

static int lock_fd {-1};
static int notify_fd {-1};
static int watch_fd {-1};
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

  // Now while we own the lock on the file, setup our notification handling
  {
    notify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

    // Watch for lock file opening and closing
    watch_fd = inotify_add_watch(notify_fd, LockPath.c_str(), IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE);
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

  bool BrokenRefCount{};
  size_t RefCount{};
  while (true) {
    constexpr size_t DATA_SIZE = (16 * (sizeof(struct inotify_event) + NAME_MAX + 1));
    char buf[DATA_SIZE];
    struct timeval tv{};
    int Ret{};

    do {
      fd_set Set{};
      FD_ZERO(&Set);
      FD_SET(notify_fd, &Set);

      // Fairly latent ten seconds
      tv.tv_sec = 10;
      tv.tv_usec = 0;

      Ret = select(notify_fd + 1, &Set, nullptr, nullptr, &tv);
      if (Ret == 0 && RefCount == 0) {
        // We don't have any more users. Clean up
        // Try to get a write lock again for the squashfs
        Ret = fcntl(lock_fd, F_SETLEASE, F_WRLCK);
        if (Ret == 0) {
          // Managed to grab the lock. Means there aren't any more users on the lock
          return;
        }
        else {
          // We weren't able to grab the lease on the lock. This means that our Refcounting
          // was broken by something and our world view is broken now
          BrokenRefCount = true;
        }
      }

      if ((BrokenRefCount && ParentShuttingDown) || ForceShutdown.load()) {
        // If our ref counting was broken and our parent is gone then try and clean up
        return;
      }
    } while (Ret == 0 || (Ret == -1 && errno == EINTR));

    if (Ret == -1) {
      return;
    }

    int Read{};
    do {
      Read = read(notify_fd, buf, DATA_SIZE);
      if (Read > 0) {
        inotify_event *Event{};
        for (char *ptr = buf;
             ptr < (buf + Read);
             ptr += (sizeof(struct inotify_event) + Event->len)) {
          Event = reinterpret_cast<inotify_event*>(ptr);
          if (Event->mask & IN_OPEN) {
            // Application opened the lock file
            ++RefCount;
          }

          if (Event->mask & (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)) {
            // Application has finally closed the lock
            // Either by choice or crashing
            --RefCount;
          }
        }
      }

    } while (Read > 0);
  }
}

void RemoveLock(std::string LockPath) {
  // Remove the lock file itself
  unlink(LockPath.c_str());

  // Clear the lease on it since we are shutting down
  fcntl(lock_fd, F_SETLEASE, F_UNLCK);

  // Now close the watch FD
  close(watch_fd);

  // Close the notify fd
  close(notify_fd);

  // Close the lock fd
  close(lock_fd);
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

  struct utsname uts{};
  uname (&uts);

  std::string LockPath = "/tmp/.FEX-";
  LockPath += std::filesystem::path(SquashFSPath).filename();
  LockPath += ".lock.";
  LockPath += uts.nodename;

  // Use lock files to ensure we aren't racing to mount multiple FSes
  auto Failure = CreateINotifyLock(LockPath, MountPath);
  if (Failure == LOCK_FAIL_FATAL) {
    return -1;
  }

  if (Failure == LOCK_FAIL_EXISTS ||
      Failure == LOCK_FAIL_CREATION_RACE) {
    // If the lock already exists
    // Then we don't need to spin up the mounts at all
    // Cleanly exit early and let FEX know it can continue
    uint64_t c = 0;
    write(pipe_wr, &c, sizeof(c));
    return 0;
  }

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
    // FEX will stall on opening the lock file until we let go of the lease
    uint64_t c = 0;
    write(pipe_wr, &c, sizeof(c));

    // Watch our lock file now for users
    WatchLock();

    RemoveLock(LockPath);

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
