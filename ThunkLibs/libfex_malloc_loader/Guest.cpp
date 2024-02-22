/*
$info$
tags: thunklibs|fex_malloc_loader
desc: Delays malloc symbol replacement until it is safe to run constructors
$end_info$
*/

#include <stdio.h>
#include <dlfcn.h>
extern "C" {
__attribute__((constructor)) static void loadlib() {
  fprintf(stderr, "Time to load mallocs\n");
  dlopen("/mnt/Work/Work/work/FEXNew/Build/Guest/libfex_malloc-guest.so", RTLD_GLOBAL | RTLD_NOW | RTLD_NODELETE | RTLD_DEEPBIND);
}
}
