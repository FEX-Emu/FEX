#include <common/GeneratorInterface.h>

#include "api.h"

template<auto>
struct fex_gen_config {};

template<typename>
struct fex_gen_type {};

template<auto, int, typename>
struct fex_gen_param {};

template<> struct fex_gen_config<GetDoubledValue> {};

template<> struct fex_gen_type<OpaqueType> : fexgen::opaque_type {};
template<> struct fex_gen_config<MakeOpaqueType> {};
template<> struct fex_gen_config<ReadOpaqueTypeData> {};
template<> struct fex_gen_config<DestroyOpaqueType> {};

template<> struct fex_gen_type<UnionType> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<MakeUnionType> {};
template<> struct fex_gen_config<GetUnionTypeA> {};
