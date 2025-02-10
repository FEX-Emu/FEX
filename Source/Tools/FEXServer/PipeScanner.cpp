// SPDX-License-Identifier: MIT
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>

namespace PipeScanner {
std::vector<int> IncomingPipes {};
void SetWaitPipe(int FD) {
  int flags = fcntl(FD, F_GETFD);
  flags |= FD_CLOEXEC;
  fcntl(FD, F_SETFD, flags);
  IncomingPipes.emplace_back(FD);
}

void ClosePipes() {
  for (auto pipe : IncomingPipes) {
    close(pipe);
  }
  IncomingPipes.clear();
}
} // namespace PipeScanner
