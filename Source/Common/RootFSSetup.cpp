#include "ConfigDefines.h"
#include "Common/FileFormatCheck.h"
#include "Common/RootFSSetup.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/LogManager.h>

#include <filesystem>
#include <fstream>

#include <poll.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <system_error>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace FEX::RootFS {

static std::fstream SquashFSLock{};
bool SanityCheckPath(std::string const &LDPath) {
  // Check if we have an directory inside our temp folder
  std::string PathUser = LDPath + "/usr";
  std::error_code ec{};
  if (!std::filesystem::exists(PathUser, ec)) {
    LogMan::Msg::D("Child couldn't mount rootfs, /usr doesn't exist");
    rmdir(LDPath.c_str());
    return false;
  }

  return true;
}

bool CheckLockExists(std::string const &LockPath, std::string *MountPath) {
  // If the lock file for a squashfs path exists the we can try
  // to open it and ref counting will keep it alive
  std::error_code ec{};
  if (std::filesystem::exists(LockPath, ec)) {
    SquashFSLock.open(LockPath, std::ios_base::in | std::ios_base::binary);
    if (SquashFSLock.is_open()) {
      // We managed to open the file. Which means the mount application has now refcounted our interaction with it
      // Extract the data in it to know where it was mounted
      std::string NewPath;
      SquashFSLock >> NewPath;
      if (NewPath.empty()) {
        // Couldn't open for whatever reason
        SquashFSLock.close();
        return false;
      }
      if (!SanityCheckPath(NewPath)) {
        // Mount doesn't exist anymore
        SquashFSLock.close();
        // Removing the dangling mount directory
        rmdir(NewPath.c_str());

        // Remove the dangling lock file
        unlink(LockPath.c_str());
        return false;
      }
      FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_ROOTFS, NewPath);
      if (MountPath) {
        *MountPath = NewPath;
      }
      return true;
    }
  }
  return false;
}

std::string GetRootFSSocketFile(std::string const &MountPath) {
  return MountPath + ".socket";
}

bool SendSocketPipe(std::string const &MountPath) {
  // Open pipes so we can send the daemon one
  int fds[2]{};
  if (pipe2(fds, 0) != 0) {
    LogMan::Msg::E("Couldn't open pipe");
    return false;
  }

  // Setup our msg header
  struct msghdr msg{};
  struct iovec iov{};
  char iov_data{};

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

  // We need to send some data in addition to the ancillary data, doesn't matter what
  iov.iov_base = &iov_data;
  iov.iov_len = sizeof(iov_data);

  // Now link to our ancilllary buffer
  msg.msg_control = AncBuf.Buffer;
  msg.msg_controllen = CMSG_SIZE;

  // Now we need to setup the ancillary buffer data. We are only sending an FD
  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;

  // We are giving the daemon the write side of the pipe
  memcpy(CMSG_DATA(cmsg), &fds[1], sizeof(int));

  // Time to open up the actual socket and send the FD over to the daemon
  // Create the initial unix socket
  int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    LogMan::Msg::D("Couldn't open AF_UNIX socket: %d %s", errno, strerror(errno));
    return false;
  }

  std::string SocketPath = GetRootFSSocketFile(MountPath);
  struct sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SocketPath.data(), sizeof(addr.sun_path));

  if (connect(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
    LogMan::Msg::D("Couldn't connect to AF_UNIX socket: %d %s", errno, strerror(errno));
    close(socket_fd);
    return false;
  }

  ssize_t ResultSend = sendmsg(socket_fd, &msg, 0);
  if (ResultSend == -1) {
    LogMan::Msg::D("Couldn't sendmsg");
    close(socket_fd);
    return false;
  }

  struct pollfd pfd{};
  pfd.fd = socket_fd;
  pfd.events = POLLIN;

  // Wait for two seconds
  struct timespec ts{};
  ts.tv_sec = 2;

  int Result = ppoll(&pfd, 1, &ts, nullptr);
  if (Result == -1 || Result == 0) {
    // didn't get ack back in time
    // Close our read pipe
    close(fds[0]);
    // close our write pipe
    close(fds[1]);

    // close socket
    close(socket_fd);
    return false;
  }

  // We've sent the message which means we're done with the socket
  close(socket_fd);

  // Close the write side of the pipe, leave read side open
  // Daemon now has a copy of the fd
  close(fds[1]);
  return true;
}

void OpenLock(std::string const LockPath) {
  SquashFSLock.open(LockPath, std::ios_base::in | std::ios_base::binary);
}

bool UpdateRootFSPath() {
  // This value may become outdated if squashfs does exist
  FEX_CONFIG_OPT(LDPath, ROOTFS);

  if (FEX::FormatCheck::IsSquashFS(LDPath())) {
    struct utsname uts{};
    uname (&uts);
    std::string LockPath = "/tmp/.FEX-";
    LockPath += std::filesystem::path(LDPath()).filename();
    LockPath += ".lock.";
    LockPath += uts.nodename;

    if (CheckLockExists(LockPath)) {
      // RootFS already exists. Nothing to do
      return true;
    }

    // Is a squashfs but not mounted
    return false;
  }

  // Nothing needing to be done
  return true;
}

std::string GetRootFSLockFile() {
  // FEX_ROOTFS needs to be the path to the squashfs, not the mount
  FEX_CONFIG_OPT(LDPath, ROOTFS);
  struct utsname uts{};
  uname (&uts);
  std::string LockPath = "/tmp/.FEX-";
  LockPath += std::filesystem::path(LDPath()).filename();
  LockPath += ".lock.";
  LockPath += uts.nodename;
  return LockPath;
}

bool Setup(char **const envp) {
  // We need to setup the rootfs here
  // If the configuration is set to use a folder then there is nothing to do
  // If it is setup to use a squashfs then we need to do something more complex

  FEX_CONFIG_OPT(LDPath, ROOTFS);
  // XXX: Disabled for now. Causing problems due to weird filesystem problems
  // Can reproduce by attempting to run an application under pressure-vessel
  // Will fail once trying to jump in to the chroot with bwrap
  // eg: FEXInterpreter ./run-in-soldier /usr/bin/ls
  if constexpr (false) {
    auto ContainerPrefix = FEXCore::Config::FindContainerPrefix();
    if (!ContainerPrefix.empty()) {
      // If we are inside of a rootfs/container then drop the rootfs path
      // Root is already our rootfs
      FEXCore::Config::Erase(FEXCore::Config::CONFIG_ROOTFS);
      return true;
    }
  }

  if (FEX::FormatCheck::IsSquashFS(LDPath())) {
    // Check if the rootfs is already mounted
    // We can do this by checking the lock file if it exists

    std::string LockPath = GetRootFSLockFile();

    // If the lock file exists and we can send the process a pipe then nothing to do
    // Otherwise we need to spin up a new mount daemon
    std::string MountPath{};
    if (CheckLockExists(LockPath, &MountPath) && SendSocketPipe(MountPath)) {
      return true;
    }

    pid_t ParentTID = ::getpid();
    std::string ParentTIDString = std::to_string(ParentTID);
    std::string Tmp = "/tmp/.FEXMount" + ParentTIDString + "-XXXXXX";
    char *TempFolder = Tmp.data();

    // Make the temporary mount folder
    if (mkdtemp(TempFolder) == nullptr) {
      LogMan::Msg::E("Couldn't create temporary mount name: %s", TempFolder);
      return false;
    }

    // Change the permissions
    if (chmod(TempFolder, 0777) != 0) {
      LogMan::Msg::E("Couldn't change permissions on temporary mount: %s", TempFolder);
      rmdir(TempFolder);
      return false;
    }

    // Open some pipes for communicating with the new processes
    int fds[2]{};
    if (pipe2(fds, 0) != 0) {
      LogMan::Msg::E("Couldn't open pipe");
      return false;
    }

    // Convert the write pipe to a string to pass to the child process
    std::string PipeString;
    PipeString = std::to_string(fds[1]);

    pid_t pid = fork();
    if (pid == 0) {
      // Child
      close(fds[0]); // Close read end of pipe
      const char *argv[5];
      argv[0] = FEX_INSTALL_PREFIX "/bin/FEXMountDaemon";
      argv[1] = LDPath().c_str();
      argv[2] = TempFolder;
      argv[3] = PipeString.c_str();
      argv[4] = nullptr;

      if (execve(argv[0], (char * const*)argv, envp) == -1) {
        // Let the parent know that we couldn't execute for some reason
        uint64_t error{1};
        write(fds[1], &error, sizeof(error));

        // Give a hopefully helpful error message for users
        LogMan::Msg::E("Couldn't execute: %s", argv[0]);
        LogMan::Msg::E("This means the squashFS rootfs won't be mounted.");
        LogMan::Msg::E("Expect errors!");
        // Destroy this fork
        exit(1);
      }
    }
    else {
      // Parent
      // Wait for the child to exit so we can check if it is mounted or not
      close(fds[1]); // Close write end of the pipe

      // Wait for a message from FEXMountDaemon
      pollfd PollFD;
      PollFD.fd = fds[0];
      PollFD.events = POLLIN;

      poll(&PollFD, 1, -1);

      // Read a value from the pipe to get an expected result
      // This will come from FEXMountDaemon or our local fork depending on results
      uint64_t ChildResult{};
      int Result = read(fds[0], &ChildResult, sizeof(ChildResult));

      if (Result != sizeof(ChildResult)) {
        LogMan::Msg::D("Spurious read error");
        return false;
      }

      if (ChildResult == 1) {
        // Error
        LogMan::Msg::D("FEXMountDaemon couldn't mount child for some reason");
        return false;
      }

      // Open the lock to let the daemon know that it has an active user
      OpenLock(LockPath);

      // Check if we have an directory inside our temp folder
      if (!SanityCheckPath(TempFolder)) {
        return false;
      }

      // Send the new FEXMountDaemon a pipe to listen to
      SendSocketPipe(TempFolder);

      // If everything has passed then we can now update the rootfs path
      FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_ROOTFS, TempFolder);
      return true;
    }
  }

  // Nothing to do
  return true;
}

void Shutdown() {
  // Close the FD so our rootfs process can refcount
  // Even if we crash the rootfs process will see a close event
  SquashFSLock.close();
}
}
