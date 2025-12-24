add_library(FEXHeaderUtils INTERFACE)

# Check for syscall support here
check_cxx_source_compiles(
  "
  #include <sched.h>
  int main() {
  return ::getcpu(nullptr, nullptr);
  }"
  HAS_SYSCALL_GETCPU)
if (HAS_SYSCALL_GETCPU)
  message(STATUS "Has getcpu helper")
  target_compile_definitions(FEXHeaderUtils INTERFACE HAS_SYSCALL_GETCPU=1)
endif ()

check_cxx_source_compiles(
  "
  #include <unistd.h>
  int main() {
  return ::gettid();
  }"
  HAS_SYSCALL_GETTID)
if (HAS_SYSCALL_GETTID)
  message(STATUS "Has gettid helper")
  target_compile_definitions(FEXHeaderUtils INTERFACE HAS_SYSCALL_GETTID=1)
endif ()

check_cxx_source_compiles(
  "
  #include <signal.h>
  int main() {
  return ::tgkill(0, 0, 0);
  }"
  HAS_SYSCALL_TGKILL)
if (HAS_SYSCALL_TGKILL)
  message(STATUS "Has tgkill helper")
  target_compile_definitions(FEXHeaderUtils INTERFACE HAS_SYSCALL_TGKILL=1)
endif ()

check_cxx_source_compiles(
  "
  #include <sys/stat.h>
  int main() {
  return ::statx(0, nullptr, 0, 0, nullptr);
  }"
  HAS_SYSCALL_STATX)
if (HAS_SYSCALL_STATX)
  message(STATUS "Has statx helper")
  target_compile_definitions(FEXHeaderUtils INTERFACE HAS_SYSCALL_STATX=1)
endif ()

check_cxx_source_compiles(
  "
  #include <stdio.h>
  int main() {
  return ::renameat2(0, nullptr, 0, nullptr, 0);
  }"
  HAS_SYSCALL_RENAMEAT2)
if (HAS_SYSCALL_RENAMEAT2)
  message(STATUS "Has renameat2 helper")
  target_compile_definitions(FEXHeaderUtils INTERFACE HAS_SYSCALL_RENAMEAT2=1)
endif ()

target_include_directories(FEXHeaderUtils INTERFACE .)
target_link_libraries(FEXHeaderUtils INTERFACE jemalloc::jemalloc)
