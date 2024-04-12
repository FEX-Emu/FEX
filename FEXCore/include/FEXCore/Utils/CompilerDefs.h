// SPDX-License-Identifier: MIT
#pragma once
// Contains general abstractions related to compilers used to build FEX.

// Specifies the minimum alignment for a variable or structure field, measured in bytes.
#define FEX_ALIGNED(alignment) __attribute__((aligned(alignment)))

// Allows annotating declarations with extra information.
#define FEX_ANNOTATE(annotation_str) __attribute__((annotate(annotation_str)))

// Makes the attributed entity have the default DSO visibility level.
// Compiler options can affect the visibility of symbols. This attribute
// overrides said changes. This gives entities external linkage.
#define FEX_DEFAULT_VISIBILITY __attribute__((visibility("default")))

// Indicates that the specified function doesn't need a function prologue/epilogue.
// emitted for it by the compiler.
#define FEX_NAKED __attribute__((naked))

// Specifies that a structure member or structure itself should have the smallest possible alignment.
#define FEX_PACKED __attribute__((packed))

// Causes execution to exit abnormally.
#define FEX_TRAP_EXECUTION FEXCore::Assert::ForcedAssert()

// Dictates to the compiler that the path this is on should not be reachable
// from normal execution control flow. If normal execution does reach this,
// then program behavior is undefined.
#define FEX_UNREACHABLE __builtin_unreachable()

namespace FEXCore::Assert {
// This function can not be inlined
[[noreturn]]
FEX_DEFAULT_VISIBILITY void ForcedAssert();
} // namespace FEXCore::Assert
