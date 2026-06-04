#include <cstdint>
#include <sys/time.h>

using gettime_type = int (*)(struct timeval* tv, struct timezone* tz);
gettime_type gettimeofday_vsyscall = (gettime_type)0xFFFF'FFFF'FF60'0000ULL;

using time_type = int (*)(time_t* tloc);
time_type time_vsyscall = (time_type)0xFFFF'FFFF'FF60'0400ULL;

using getcpu_type = int (*)(uint32_t* cpu, uint32_t* node);
getcpu_type getcpu_vsyscall = (getcpu_type)0xFFFF'FFFF'FF60'0800ULL;

int main() {
  // This test just ensures that these vsyscalls execute without crashing.
  // FEX has broken vsyscalls periodically.
  timeval tv {};
  gettimeofday_vsyscall(&tv, nullptr);

  time_t tloc {};
  time_vsyscall(&tloc);

  uint32_t cpu, node;
  getcpu_vsyscall(&cpu, &node);

  return 0;
}
