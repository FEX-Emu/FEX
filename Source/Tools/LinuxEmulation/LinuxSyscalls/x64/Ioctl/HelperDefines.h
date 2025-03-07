// SPDX-License-Identifier: MIT
#pragma once

#define STRINGY2(x, y) x##y
#define STRINGY(x, y) STRINGY2(x, y)

#define STRINGY12(x) STRINGY11(x)
#define STRINGY11(x) #x
#define STRINGY1(x) STRINGY12(x)

#ifndef _BASIC_META
// Meta typedef variable in unnamed and matches upstream
// Use this for the super basic ioctl passthrough path
#define _BASIC_META(x)                   \
  __attribute__((annotate("fex-match"))) \
  __attribute__((annotate("ioctl-alias-x86_64-_" #x STRINGY1(__LINE__)))) typedef uint8_t STRINGY(_##x, __LINE__)[x];
#endif

#ifndef _BASIC_META_VAR
// This is similar to _BASIC_META except that it allows you to pass variadic arguments to the original ioctl definition
#define _BASIC_META_VAR(x, args...)      \
  __attribute__((annotate("fex-match"))) \
  __attribute__((annotate("ioctl-alias-x86_64-_" #x STRINGY1(__LINE__)))) typedef uint8_t STRINGY(_##x, __LINE__)[x(args)];
#endif

#ifndef _CUSTOM_META
// IOCTL doesn't match across architecture
// Generates a FEX_<name> version of the ioctl with custom ioctl definition
// eg: _CUSTOM_META(DRM_IOCTL_AMDGPU_GEM_METADATA, DRM_IOWR(DRM_COMMAND_BASE + DRM_AMDGPU_GEM_METADATA, FEX::HLE::x64::AMDGPU::fex_drm_amdgpu_gem_metadata));
// Allows you to effectively pass in the original ioctl definition with custom type replacing the upstream type
#define _CUSTOM_META(name, ioctl_num)                                                              \
  typedef uint8_t _meta_##name[name];                                                              \
  __attribute__((annotate("ioctl-alias-x86_64-_meta_" #name))) typedef uint8_t _##name[ioctl_num]; \
  constexpr static uint32_t FEX_##name = ioctl_num;
#endif

#ifndef _CUSTOM_META_MATCH
// IOCTL doesn't match across architecture
// Generates a FEX_<name> version of the ioctl with custom ioctl definition
// eg: _CUSTOM_META(DRM_IOCTL_AMDGPU_GEM_METADATA, DRM_IOWR(DRM_COMMAND_BASE + DRM_AMDGPU_GEM_METADATA, FEX::HLE::x64::AMDGPU::fex_drm_amdgpu_gem_metadata));
// Allows you to effectively pass in the original ioctl definition with custom type replacing the upstream type
#define _CUSTOM_META_MATCH(name, ioctl_num)                                  \
  typedef uint8_t _meta_##name[ioctl_num];                                   \
  __attribute__((annotate("fex-match"))) typedef uint8_t _##name[ioctl_num]; \
  constexpr static uint32_t FEX_##name = ioctl_num;
#endif

#ifndef _CUSTOM_META_OFFSET
// Same as _CUSTOM_META but allows you to define multiple types from an offset
// Required to have an ioctl covering a range which some ioctls do
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)                                                        \
  typedef uint8_t _meta_##name[ioctl_num + offset];                                                         \
  __attribute__((annotate("ioctl-alias-x86_64-_meta_" #name))) typedef uint8_t _##name[ioctl_num + offset]; \
  constexpr static uint32_t FEX_##name = ioctl_num + offset;
#endif
