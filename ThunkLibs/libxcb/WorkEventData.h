#pragma once
#include <cstdint>

#ifndef GUEST_THUNK_LIBRARY
#include "../include/common/Host.h"
#endif

struct CBWork {
#ifdef GUEST_THUNK_LIBRARY
  uintptr_t cb;
#else
  fex_guest_function_ptr cb;
#endif
  void *argsv;
};

