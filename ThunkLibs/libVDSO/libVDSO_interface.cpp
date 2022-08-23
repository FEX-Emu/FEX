#include <common/GeneratorInterface.h>

#include <sched.h>
#include <sys/time.h>
#include <time.h>

template<auto>
struct fex_gen_config {
};

template<> struct fex_gen_config<time> {};
template<> struct fex_gen_config<gettimeofday> {};
template<> struct fex_gen_config<clock_gettime> {};
template<> struct fex_gen_config<clock_getres> {};
template<> struct fex_gen_config<getcpu> {};
