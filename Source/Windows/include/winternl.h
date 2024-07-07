// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) the Wine project

#pragma once

#include_next <winternl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NtCurrentProcess() ((HANDLE) ~(ULONG_PTR)0)

#define WOW64_TLS_MAX_NUMBER 19

#ifdef _M_ARM_64EC
typedef struct _CHPE_V2_CPU_AREA_INFO {
  BOOLEAN InSimulation;             /* 000 */
  BOOLEAN InSyscallCallback;        /* 001 */
  ULONG64 EmulatorStackBase;        /* 008 */
  ULONG64 EmulatorStackLimit;       /* 010 */
  ARM64EC_NT_CONTEXT* ContextAmd64; /* 018 */
  ULONG* SuspendDoorbell;           /* 020 */
  ULONG64 LoadingModuleModflag;     /* 028 */
  void* EmulatorData[4];            /* 030 */
  ULONG64 EmulatorDataInline;       /* 050 */
} CHPE_V2_CPU_AREA_INFO, *PCHPE_V2_CPU_AREA_INFO;
#endif

typedef struct _THREAD_BASIC_INFORMATION {
  NTSTATUS ExitStatus;
  PVOID TebBaseAddress;
  CLIENT_ID ClientId;
  ULONG_PTR AffinityMask;
  LONG Priority;
  LONG BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

/* System Information Class 0x01 */

typedef struct _SYSTEM_CPU_INFORMATION {
  USHORT ProcessorArchitecture;
  USHORT ProcessorLevel;
  USHORT ProcessorRevision;
  USHORT MaximumProcessors;
  ULONG ProcessorFeatureBits;
} SYSTEM_CPU_INFORMATION, *PSYSTEM_CPU_INFORMATION;

/* definitions of bits in the Feature set for the x86 processors */
#define CPU_FEATURE_VME 0x00000005    /* Virtual 86 Mode Extensions */
#define CPU_FEATURE_TSC 0x00000002    /* Time Stamp Counter available */
#define CPU_FEATURE_CMOV 0x00000008   /* Conditional Move instruction*/
#define CPU_FEATURE_PGE 0x00000014    /* Page table Entry Global bit */
#define CPU_FEATURE_PSE 0x00000024    /* Page Size Extension */
#define CPU_FEATURE_MTRR 0x00000040   /* Memory Type Range Registers */
#define CPU_FEATURE_CX8 0x00000080    /* Compare and eXchange 8 byte instr. */
#define CPU_FEATURE_MMX 0x00000100    /* Multi Media eXtensions */
#define CPU_FEATURE_X86 0x00000200    /* seems to be always ON, on the '86 */
#define CPU_FEATURE_PAT 0x00000400    /* Page Attribute Table */
#define CPU_FEATURE_FXSR 0x00000800   /* FXSAVE and FXSTORE instructions */
#define CPU_FEATURE_SEP 0x00001000    /* SYSENTER and SYSEXIT instructions */
#define CPU_FEATURE_SSE 0x00002000    /* SSE extensions (ext. MMX) */
#define CPU_FEATURE_3DNOW 0x00004000  /* 3DNOW instructions available */
#define CPU_FEATURE_SSE2 0x00010000   /* SSE2 extensions (XMMI64) */
#define CPU_FEATURE_DS 0x00020000     /* Debug Store */
#define CPU_FEATURE_HTT 0x00040000    /* Hyper Threading Technology */
#define CPU_FEATURE_SSE3 0x00080000   /* SSE3 extensions */
#define CPU_FEATURE_CX128 0x00100000  /* cmpxchg16b instruction */
#define CPU_FEATURE_XSAVE 0x00800000  /* XSAVE instructions */
#define CPU_FEATURE_2NDLEV 0x04000000 /* Second-level address translation */
#define CPU_FEATURE_VIRT 0x08000000   /* Virtualization support */
#define CPU_FEATURE_RDFS 0x10000000   /* RDFSBASE etc. instructions */
#define CPU_FEATURE_NX 0x20000000     /* Data execution prevention */

/* FIXME: following values are made up, actual flags are unknown */
#define CPU_FEATURE_SSSE3 0x00008000 /* SSSE3 instructions */
#define CPU_FEATURE_SSE41 0x01000000 /* SSE41 instructions */
#define CPU_FEATURE_SSE42 0x02000000 /* SSE42 instructions */
#define CPU_FEATURE_AVX 0x40000000   /* AVX instructions */
#define CPU_FEATURE_AVX2 0x80000000  /* AVX2 instructions */
#define CPU_FEATURE_PAE 0x00200000
#define CPU_FEATURE_DAZ 0x00400000

typedef enum _MEMORY_INFORMATION_CLASS {
  MemoryBasicInformation,
  MemoryWorkingSetInformation,
  MemoryMappedFilenameInformation,
  MemoryRegionInformation,
  MemoryWorkingSetExInformation,
  MemorySharedCommitInformation,
  MemoryImageInformation,
  MemoryRegionInformationEx,
  MemoryPrivilegedBasicInformation,
  MemoryEnclaveImageInformation,
  MemoryBasicInformationCapped,
  MemoryPhysicalContiguityInformation,
  MemoryBadInformation,
  MemoryBadInformationAllProcesses,
#ifdef __WINESRC__
  MemoryWineUnixFuncs = 1000,
  MemoryWineUnixWow64Funcs,
#endif
} MEMORY_INFORMATION_CLASS;

NTSTATUS WINAPI Wow64SystemServiceEx(UINT, UINT*);
void WINAPI Wow64ProcessPendingCrossProcessItems(void);

NTSTATUS WINAPI RtlWow64SetThreadContext(HANDLE, const WOW64_CONTEXT*);
NTSTATUS WINAPI RtlWow64GetThreadContext(HANDLE, WOW64_CONTEXT*);
NTSTATUS WINAPI RtlWow64GetCurrentCpuArea(USHORT*, void**, void**);

NTSTATUS WINAPI NtSuspendThread(HANDLE, PULONG);
NTSTATUS WINAPI NtGetContextThread(HANDLE, CONTEXT*);
NTSTATUS WINAPI NtContinue(PCONTEXT, BOOLEAN);
NTSTATUS WINAPI NtAllocateVirtualMemory(HANDLE, PVOID*, ULONG_PTR, SIZE_T*, ULONG, ULONG);
NTSTATUS WINAPI NtQueryVirtualMemory(HANDLE, LPCVOID, MEMORY_INFORMATION_CLASS, PVOID, SIZE_T, SIZE_T*);
NTSTATUS WINAPI NtProtectVirtualMemory(HANDLE, PVOID*, SIZE_T*, ULONG, ULONG*);

#ifdef __cplusplus
}
#endif
