// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace PipeScanner {
std::vector<int> IncomingPipes {};

// Scan and store any pipe files.
// This will capture all pipe files so needs to be executed early.
// This ensures we find any pipe files from execve for waiting FEXInterpreters.
void ScanForPipes() {
  DIR* fd = opendir("/proc/self/fd");
  if (fd) {
    struct dirent* dir {};
    do {
      dir = readdir(fd);
      if (dir) {
        char* end {};
        int open_fd = std::strtol(dir->d_name, &end, 8);
        if (end != dir->d_name) {
          struct stat stat {};
          int result = fstat(open_fd, &stat);
          if (result == -1) {
            continue;
          }
          if (stat.st_mode & S_IFIFO) {
            // Close any incoming pipes
            IncomingPipes.emplace_back(open_fd);
          }
        }
      }
    } while (dir);

    closedir(fd);
  }
}

void ClosePipes() {
  for (auto pipe : IncomingPipes) {
    close(pipe);
  }
  IncomingPipes.clear();
}
} // namespace PipeScanner
