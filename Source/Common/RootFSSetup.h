#pragma once
#include <string>

namespace FEX::RootFS {
  // Updates the RootFS path in the case of squashfs
  // Doesn't mount the rootfs if it doesn't exist
  // Returns true if the rootfs is accessible regularly, mounted or folder
  bool UpdateRootFSPath();
  // Returns where the rootfs lock file lives even if the squashfs isn't mounted
  std::string GetRootFSLockFile();
  // Returns the socket file for a mount path
  std::string GetRootFSSocketFile(std::string const &MountPath);
  // Checks if the rootfs lock exists
  bool CheckLockExists(std::string const &LockPath, std::string *MountPath = nullptr);
  bool Setup(char **const envp);
  void Shutdown();
}
