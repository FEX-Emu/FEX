// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) the Wine project

#pragma once

#include_next <winnt.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _IMAGE_LOAD_CONFIG_CODE_INTEGRITY {
  WORD Flags;
  WORD Catalog;
  DWORD CatalogOffset;
  DWORD Reserved;
} IMAGE_LOAD_CONFIG_CODE_INTEGRITY, *PIMAGE_LOAD_CONFIG_CODE_INTEGRITY;

typedef struct __IMAGE_LOAD_CONFIG_DIRECTORY64 {
  DWORD Size; /* 000 */
  DWORD TimeDateStamp;
  WORD MajorVersion;
  WORD MinorVersion;
  DWORD GlobalFlagsClear;
  DWORD GlobalFlagsSet; /* 010 */
  DWORD CriticalSectionDefaultTimeout;
  ULONGLONG DeCommitFreeBlockThreshold;
  ULONGLONG DeCommitTotalFreeThreshold; /* 020 */
  ULONGLONG LockPrefixTable;
  ULONGLONG MaximumAllocationSize; /* 030 */
  ULONGLONG VirtualMemoryThreshold;
  ULONGLONG ProcessAffinityMask; /* 040 */
  DWORD ProcessHeapFlags;
  WORD CSDVersion;
  WORD DependentLoadFlags;
  ULONGLONG EditList; /* 050 */
  ULONGLONG SecurityCookie;
  ULONGLONG SEHandlerTable; /* 060 */
  ULONGLONG SEHandlerCount;
  ULONGLONG GuardCFCheckFunctionPointer; /* 070 */
  ULONGLONG GuardCFDispatchFunctionPointer;
  ULONGLONG GuardCFFunctionTable; /* 080 */
  ULONGLONG GuardCFFunctionCount;
  DWORD GuardFlags; /* 090 */
  IMAGE_LOAD_CONFIG_CODE_INTEGRITY CodeIntegrity;
  ULONGLONG GuardAddressTakenIatEntryTable; /* 0a0 */
  ULONGLONG GuardAddressTakenIatEntryCount;
  ULONGLONG GuardLongJumpTargetTable; /* 0b0 */
  ULONGLONG GuardLongJumpTargetCount;
  ULONGLONG DynamicValueRelocTable; /* 0c0 */
  ULONGLONG CHPEMetadataPointer;
  ULONGLONG GuardRFFailureRoutine; /* 0d0 */
  ULONGLONG GuardRFFailureRoutineFunctionPointer;
  DWORD DynamicValueRelocTableOffset; /* 0e0 */
  WORD DynamicValueRelocTableSection;
  WORD Reserved2;
  ULONGLONG GuardRFVerifyStackPointerFunctionPointer;
  DWORD HotPatchTableOffset; /* 0f0 */
  DWORD Reserved3;
  ULONGLONG EnclaveConfigurationPointer;
  ULONGLONG VolatileMetadataPointer; /* 100 */
  ULONGLONG GuardEHContinuationTable;
  ULONGLONG GuardEHContinuationCount; /* 110 */
  ULONGLONG GuardXFGCheckFunctionPointer;
  ULONGLONG GuardXFGDispatchFunctionPointer; /* 120 */
  ULONGLONG GuardXFGTableDispatchFunctionPointer;
  ULONGLONG CastGuardOsDeterminedFailureMode; /* 130 */
  ULONGLONG GuardMemcpyFunctionPointer;
} _IMAGE_LOAD_CONFIG_DIRECTORY64, *_PIMAGE_LOAD_CONFIG_DIRECTORY64;

typedef struct _IMAGE_CHPE_RANGE_ENTRY {
  union {
    ULONG StartOffset;
    struct {
      ULONG NativeCode  : 1;
      ULONG AddressBits : 31;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  ULONG Length;
} IMAGE_CHPE_RANGE_ENTRY, *PIMAGE_CHPE_RANGE_ENTRY;

typedef struct _IMAGE_ARM64EC_METADATA {
  ULONG Version;
  ULONG CodeMap;
  ULONG CodeMapCount;
  ULONG CodeRangesToEntryPoints;
  ULONG RedirectionMetadata;
  ULONG __os_arm64x_dispatch_call_no_redirect;
  ULONG __os_arm64x_dispatch_ret;
  ULONG __os_arm64x_dispatch_call;
  ULONG __os_arm64x_dispatch_icall;
  ULONG __os_arm64x_dispatch_icall_cfg;
  ULONG AlternateEntryPoint;
  ULONG AuxiliaryIAT;
  ULONG CodeRangesToEntryPointsCount;
  ULONG RedirectionMetadataCount;
  ULONG GetX64InformationFunctionPointer;
  ULONG SetX64InformationFunctionPointer;
  ULONG ExtraRFETable;
  ULONG ExtraRFETableSize;
  ULONG __os_arm64x_dispatch_fptr;
  ULONG AuxiliaryIATCopy;
  ULONG __os_arm64x_helper0;
  ULONG __os_arm64x_helper1;
  ULONG __os_arm64x_helper2;
  ULONG __os_arm64x_helper3;
  ULONG __os_arm64x_helper4;
  ULONG __os_arm64x_helper5;
  ULONG __os_arm64x_helper6;
  ULONG __os_arm64x_helper7;
  ULONG __os_arm64x_helper8;
} IMAGE_ARM64EC_METADATA;

typedef struct _IMAGE_ARM64EC_REDIRECTION_ENTRY {
  ULONG Source;
  ULONG Destination;
} IMAGE_ARM64EC_REDIRECTION_ENTRY;

typedef struct _IMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT {
  ULONG StartRva;
  ULONG EndRva;
  ULONG EntryPoint;
} IMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT;


#ifdef __cplusplus
}
#endif
