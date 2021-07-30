#include "ConfigDefines.h"
#include "Common/Config.h"
#include "Common/FileFormatCheck.h"

#include <FEXCore/Config/Config.h>

#include <filesystem>
#include <fstream>

#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>

namespace FEX::RootFS {

static std::fstream SquashFSLock{};
bool SanityCheckPath(std::string const &LDPath) {
// Check if we have an directory inside our temp folder
  std::string PathUser = LDPath + "/usr";
  if (!std::filesystem::exists(PathUser)) {
    LogMan::Msg::D("Child couldn't mount rootfs, /usr doesn't exist");
    rmdir(LDPath.c_str());
    return false;
  }

  return true;
}

bool CheckLockExists(std::string const LockPath) {
  // If the lock file for a squashfs path exists the we can try
  // to open it and ref counting will keep it alive
  if (std::filesystem::exists(LockPath)) {
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
      return true;
    }
  }
  return false;
}

void OpenLock(std::string const LockPath) {
  SquashFSLock.open(LockPath, std::ios_base::in | std::ios_base::binary);
}

bool Setup(char **const envp) {
  // We need to setup the rootfs here
  // If the configuration is set to use a folder then there is nothing to do
  // If it is setup to use a squashfs then we need to do something more complex

  FEX_CONFIG_OPT(LDPath, ROOTFS);
  if (FEX::FormatCheck::IsSquashFS(LDPath())) {
    // Check if the rootfs is already mounted
    // We can do this by checking the lock file if it exists

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
      const char *argv[6];
      argv[0] = FEX_INSTALL_PREFIX "/bin/FEXMountDaemon";
      argv[1] = LDPath().c_str();
      argv[2] = TempFolder;
      argv[3] = ParentTIDString.c_str();
      argv[4] = PipeString.c_str();
      argv[5] = nullptr;

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
