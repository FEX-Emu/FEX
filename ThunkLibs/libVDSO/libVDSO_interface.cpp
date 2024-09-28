#include <common/GeneratorInterface.h>

#include <sched.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "Types.h"

template<auto>
struct fex_gen_config {};

template<>
struct fex_gen_config<time> {};
template<>
struct fex_gen_config<gettimeofday> {};
template<>
struct fex_gen_config<clock_gettime> {};
template<>
struct fex_gen_config<clock_getres> {};
template<>
struct fex_gen_config<getcpu> {};

#if __SIZEOF_POINTER__ == 4
extern int clock_gettime64(clockid_t __clock_id, struct timespec64* __tp) __THROW;
template<>
struct fex_gen_config<clock_gettime64> {};
#else
extern ssize_t getrandom(void* buffer, size_t len, uint32_t flags, void* opaque_state, size_t opaque_len);
template<>
struct fex_gen_config<getrandom> {};
#endif
