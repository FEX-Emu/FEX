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

template<> struct fex_gen_config<MakeReorderingType> {};
template<> struct fex_gen_config<GetReorderingTypeMember> {};
template<> struct fex_gen_config<GetReorderingTypeMemberWithoutRepacking> {};
template<> struct fex_gen_param<GetReorderingTypeMemberWithoutRepacking, 0, const ReorderingType*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<ModifyReorderingTypeMembers> {};

template<> struct fex_gen_config<QueryOffsetOf> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<QueryOffsetOf, 0, ReorderingType*> : fexgen::ptr_passthrough {};

template<> struct fex_gen_config<&CustomRepackedType::data> : fexgen::custom_repack {};
template<> struct fex_gen_config<RanCustomRepack> {};

template<> struct fex_gen_config<FunctionWithDivergentSignature> {};

template<> struct fex_gen_config<&TestBaseStruct::Next> : fexgen::custom_repack {};
template<> struct fex_gen_config<&TestStruct1::Next> : fexgen::custom_repack {};
template<> struct fex_gen_config<&TestStruct2::Next> : fexgen::custom_repack {};

template<> struct fex_gen_config<ReadData1> {};
