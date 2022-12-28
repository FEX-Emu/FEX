#include <common/GeneratorInterface.h>

#include "api.h"

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

template<typename>
struct fex_gen_type {};

//void SetAsyncCallback(void(*)());
//template<> struct fex_gen_config<SetAsyncCallback> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

// TODO: Only needs to be customized on 32-bit
template<> struct fex_gen_config<&TestStruct1::Next> : fexgen::custom_repack {};
template<> struct fex_gen_config<&TestStruct2::Next> : fexgen::custom_repack {};

template<> struct fex_gen_config<TestFunction> : fexgen::custom_guest_entrypoint, fexgen::custom_host_impl {};
