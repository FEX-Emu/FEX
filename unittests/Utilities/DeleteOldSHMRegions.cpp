#include <cstdint>
#include <fstream>
#include <stdio.h>
#include <sys/shm.h>
#include <unistd.h>

static bool ctime_is_old(uint64_t ctime) {
  time_t curtime;
  time(&curtime);

  // If it is older than ten minutes.
  return (curtime - ctime) > 600;
}

int main() {
  // key      shmid perms                  size  cpid  lpid nattch   uid   gid  cuid  cgid      atime      dtime      ctime                   rss                  swap
  //	 0     360448   777                  4096 165187 165187      0  1002  1002  1002  1002 1679659857 1679659857 1679659857                  4096                     0
  //	 0      32769   777                  4096 153814 153814      0  1002  1002  1002  1002 1676490841 1676490841 1676490841                     0                  4096
  std::ifstream fs{"/proc/sysvipc/shm", std::fstream::binary};
  std::string Line;

  // Remove first line
  std::getline(fs, Line);

  const auto current_uid = getuid();
  while (std::getline(fs, Line)) {
    if (fs.eof()) break;
    int shmid;
    int uid;
    int attach;
    uint64_t ctime;
    if (sscanf(Line.c_str(), "%*d %d %*d %*d %*d %*d %d %d %*d %*d %*d %*d %*d %ld", &shmid, &attach, &uid, &ctime) == 4) {
      // If the UID matches and nothing is attached AND it is old then delete it.
      if (uid == current_uid &&
          attach == 0 &&
          ctime_is_old(ctime)) {
        shmctl(shmid, IPC_RMID, nullptr);
      }
    }
  }
  return 0;

}
