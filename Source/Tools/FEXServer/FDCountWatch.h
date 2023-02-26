#pragma once
#include <sys/types.h>

namespace FDCountWatch {
  void GetMaxFDs();
  void IncrementFDCountAndCheckLimits(ssize_t Num);
}
