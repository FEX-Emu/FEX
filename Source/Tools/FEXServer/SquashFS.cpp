// SPDX-License-Identifier: MIT
#include "Common/FEXServerClient.h"
#include "Common/FileFormatCheck.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>

#include <fcntl.h>
#include <filesystem>
#include <sys/poll.h>
#include <sys/wait.h>
#include <thread>

namespace SquashFS {

constexpr int USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;
int ServerRootFSLockFD {-1};
int FuseMountPID {};
fextl::string MountFolder {};

void ShutdownImagePID() {
  if (FuseMountPID) {
    FHU::Syscalls::tgkill(FuseMountPID, FuseMountPID, SIGINT);
  }
}

bool InitializeSquashFSPipe() {
  auto RootFSLockFile = FEXServerClient::GetServerRootFSLockFile();

  int Ret = open(RootFSLockFile.c_str(), O_CREAT | O_RDWR | O_TRUNC | O_EXCL | O_CLOEXEC, USER_PERMS);
  ServerRootFSLockFD = Ret;
  if (Ret == -1 && errno == EEXIST) {
    // If the fifo exists then it might be a stale connection.
    // Check the lock status to see if another process is still alive.
    ServerRootFSLockFD = open(RootFSLockFile.c_str(), O_RDWR | O_CLOEXEC, USER_PERMS);
    if (ServerRootFSLockFD != -1) {
      // Now that we have opened the file, try to get a write lock.
      flock lk {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
      };
      Ret = fcntl(ServerRootFSLockFD, F_SETLK, &lk);

      if (Ret != -1) {
        // Write lock was gained, we can now continue onward.
      } else {
        // We couldn't get a write lock, this means that another process already owns a lock on the fifo
        close(ServerRootFSLockFD);
        ServerRootFSLockFD = -1;
        return false;
      }
    } else {
      // File couldn't get opened even though it existed?
      // Must have raced something here.
      return false;
    }
  } else if (Ret == -1) {
    // Unhandled error.
    LogMan::Msg::EFmt("[FEXServer] Unable to create FEXServer RootFS lock file at: {} {} {}", RootFSLockFile, errno, strerror(errno));
    return false;
  } else {
    // FIFO file was created. Try to get a write lock
    flock lk {
      .l_type = F_WRLCK,
      .l_whence = SEEK_SET,
      .l_start = 0,
      .l_len = 0,
    };
    Ret = fcntl(ServerRootFSLockFD, F_SETLK, &lk);

    if (Ret == -1) {
      // Couldn't get a write lock, something else must have got it
      close(ServerRootFSLockFD);
      ServerRootFSLockFD = -1;
      return false;
    }
  }

  return true;
}

bool DowngradeRootFSPipeToReadLock() {
  flock lk {
    .l_type = F_RDLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = 0,
  };
  int Ret = fcntl(ServerRootFSLockFD, F_SETLK, &lk);

  if (Ret == -1) {
    // This shouldn't occur
    LogMan::Msg::EFmt("[FEXServer] Unable to downgrade a rootfs write lock to a read lock {} {}", errno, strerror(errno));
    close(ServerRootFSLockFD);
    ServerRootFSLockFD = -1;
    return false;
  }

  return true;
}

bool MountRootFSImagePath(const fextl::string& SquashFS, bool EroFS) {
  pid_t ParentTID = ::getpid();
  MountFolder = fextl::fmt::format("{}/.FEXMount{}-XXXXXX", FEXServerClient::GetServerMountFolder(), ParentTID);
  char* MountFolderStr = MountFolder.data();

  // Make the temporary mount folder
  if (mkdtemp(MountFolderStr) == nullptr) {
    LogMan::Msg::EFmt("[FEXServer] Couldn't create temporary mount name: {}", MountFolder);
    return false;
  }

  // Change the permissions
  if (chmod(MountFolderStr, 0777) != 0) {
    LogMan::Msg::EFmt("[FEXServer] Couldn't change permissions on temporary mount: {}", MountFolder);
    rmdir(MountFolderStr);
    return false;
  }

  // Create local FDs so our internal forks can communicate
  int fds[2];
  pipe2(fds, 0);

  int pid = fork();
  if (pid == 0) {
    // Child
    close(fds[0]); // Close read side
    const char* argv[4];
    argv[0] = EroFS ? "erofsfuse" : "squashfuse";
    argv[1] = SquashFS.c_str();
    argv[2] = MountFolder.c_str();
    argv[3] = nullptr;

    // Try and execute {erofsfuse, squashfuse} to mount our rootfs
    if (execvpe(argv[0], (char* const*)argv, environ) == -1) {
      // Give a hopefully helpful error message for users
      LogMan::Msg::EFmt("[FEXServer] '{}' Couldn't execute for some reason: {} {}\n", argv[0], errno, strerror(errno));
      LogMan::Msg::EFmt("[FEXServer] To mount squashfs rootfs files you need {} installed\n", argv[0]);
      LogMan::Msg::EFmt("[FEXServer] Check your FUSE setup.\n");

      // Let the parent know that we couldn't execute for some reason
      uint64_t error {1};
      write(fds[1], &error, sizeof(error));

      // End the child
      exit(1);
    }
  } else {
    FuseMountPID = pid;
    // Parent
    // Wait for the child to exit
    // This will happen with execvpe of squashmount or exit on failure
    while (waitpid(pid, nullptr, 0) == -1 && errno == EINTR)
      ;

    // Check the child pipe for messages
    pollfd PollFD;
    PollFD.fd = fds[0];
    PollFD.events = POLLIN;

    int Result = poll(&PollFD, 1, 0);

    if (Result == 1 && PollFD.revents & POLLIN) {
      // Child couldn't execvpe for whatever reason
      // Remove the mount path and leave Just in case it was created
      rmdir(MountFolderStr);

      // Close the pipe now
      close(fds[0]);

      LogMan::Msg::EFmt("[FEXServer] Couldn't mount squashfs\n");
      return false;
    }

    // Close the pipe now
    close(fds[0]);
  }

  // Write to the lock file where we are mounted
  write(ServerRootFSLockFD, MountFolder.c_str(), MountFolder.size());
  fdatasync(ServerRootFSLockFD);

  return true;
}

void UnmountRootFS() {
  FEX_CONFIG_OPT(LDPath, ROOTFS);
  if (!FEX::FormatCheck::IsSquashFS(LDPath()) && !FEX::FormatCheck::IsEroFS(LDPath())) {
    return;
  }

  SquashFS::ShutdownImagePID();

  // Handle final mount removal
  // fusermount for unmounting the mountpoint, then the {erfsfuse, squashfuse} will exit automatically
  int pid = fork();

  if (pid == 0) {
    const char* argv[5];
    argv[0] = "fusermount";
    argv[1] = "-u";
    argv[2] = "-q";
    argv[3] = MountFolder.c_str();
    argv[4] = nullptr;

    if (execvp(argv[0], (char* const*)argv) == -1) {
      fprintf(stderr, "fusermount failed to execute. You may have an mount living at '%s' to clean up now\n", MountFolder.c_str());
      fprintf(stderr, "Try `%s %s %s %s`\n", argv[0], argv[1], argv[2], argv[3]);
      exit(1);
    }
  } else {
    // Wait for fusermount to leave
    while (waitpid(pid, nullptr, 0) == -1 && errno == EINTR)
      ;

    // Remove the mount path
    rmdir(MountFolder.c_str());

    // Remove the rootfs lock file
    auto RootFSLockFile = FEXServerClient::GetServerRootFSLockFile();
    unlink(RootFSLockFile.c_str());
  }
}

bool InitializeSquashFS() {
  FEX_CONFIG_OPT(LDPath, ROOTFS);

  MountFolder = LDPath();

  bool IsSquashFS {false};
  bool IsEroFS {false};

  // Check if the image is an EroFS
  IsEroFS = FEX::FormatCheck::IsEroFS(MountFolder);

  if (!IsEroFS) {
    // Check if the image is an SquashFS
    IsSquashFS = FEX::FormatCheck::IsSquashFS(MountFolder);
  }

  if (!IsSquashFS && !IsEroFS) {
    // If this isn't a rootfs image then we have nothing to do here
    return true;
  }

  if (!InitializeSquashFSPipe()) {
    LogMan::Msg::EFmt("[FEXServer] Couldn't initialize SquashFSPipe");
    return false;
  }

  // Setup rootfs here
  if (!MountRootFSImagePath(LDPath(), IsEroFS)) {
    LogMan::Msg::EFmt("[FEXServer] Couldn't mount squashfs path");
    return false;
  }

  if (!DowngradeRootFSPipeToReadLock()) {
    LogMan::Msg::EFmt("[FEXServer] Couldn't downgrade read lock");
    return false;
  }

  return true;
}

const fextl::string& GetMountFolder() {
  return MountFolder;
}
} // namespace SquashFS
