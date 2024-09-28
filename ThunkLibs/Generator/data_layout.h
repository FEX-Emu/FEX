#pragma once

#include "analysis.h"

#include <clang/Frontend/FrontendAction.h>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

std::unordered_map<const clang::Type*, TypeInfo>
ComputeDataLayout(const clang::ASTContext& context, const std::unordered_map<const clang::Type*, AnalysisAction::RepackedType>& types);

// Convert the output of ComputeDataLayout to a format that isn't tied to a libclang session.
// As a consequence, type information is indexed by type name instead of clang::Type.
ABI GetStableLayout(const clang::ASTContext& context, const std::unordered_map<const clang::Type*, TypeInfo>& data_layout);

/**
 * Returns the type of the given name, but replaces any mentions of integer
 * types with fixed-size equivalents.
 *
 * Examples:
 * - int -> int32_t
 * - unsigned long long* -> uint64_t*
 * - MyStruct -> MyStruct (no change)
 */
std::string GetTypeNameWithFixedSizeIntegers(clang::ASTContext&, clang::QualType);

enum class TypeCompatibility {
  Full,       // Type has matching data layout across architectures
  Repackable, // Type has different data layout but can be repacked automatically
  None,       // Type has different data layout and cannot be repacked automatically
};

class DataLayoutCompareAction : public AnalysisAction {
public:
  DataLayoutCompareAction(const ABI& guest_abi)
    : AnalysisAction(guest_abi)
    , guest_abi(guest_abi) {}

  TypeCompatibility GetTypeCompatibility(const clang::ASTContext&, const clang::Type*,
                                         const std::unordered_map<const clang::Type*, TypeInfo> host_abi,
                                         std::unordered_map<const clang::Type*, TypeCompatibility>& type_compat);

  FuncPtrInfo LookupGuestFuncPtrInfo(const char* funcptr_id);

protected:
  const ABI& guest_abi;
};
