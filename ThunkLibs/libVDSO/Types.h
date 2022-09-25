#pragma once
#include <cstdint>

struct timespec64 {
  int64_t tv_sec;
  int64_t tv_nsec;
};
