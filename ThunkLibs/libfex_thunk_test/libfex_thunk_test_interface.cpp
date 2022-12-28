#include <common/GeneratorInterface.h>

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

void SetAsyncCallback(void(*)());
template<> struct fex_gen_config<SetAsyncCallback> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
