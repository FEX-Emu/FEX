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

  // Returns the string to the rootfs path if it exists
  // If unconfigured: Return empty string
  // If regular directory: Return directory
  // If squashfs: Only returns tmpfs directory if mounted, otherwise empty string
  std::string GetRootFSPathIfExists();

  // Checks if the rootfs lock exists
  bool CheckLockExists(std::string const &LockPath, std::string *MountPath = nullptr);
  bool Setup(char **const envp, uint32_t TryCount = 0);
  void Shutdown();
}
