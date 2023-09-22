#include <common/GeneratorInterface.h>

extern "C" {
#include <X11/xshmfence.h>
}

template<auto>
struct fex_gen_config {
    unsigned version = 1;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<xshmfence> : fexgen::opaque_type {};

template<> struct fex_gen_config<xshmfence_trigger> {};
template<> struct fex_gen_config<xshmfence_await> {};
template<> struct fex_gen_config<xshmfence_query> {};
template<> struct fex_gen_config<xshmfence_reset> {};
template<> struct fex_gen_config<xshmfence_alloc_shm> {};
template<> struct fex_gen_config<xshmfence_map_shm> {};
template<> struct fex_gen_config<xshmfence_unmap_shm> {};
