// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) the Wine project

#pragma once

#include_next <winternl.h>
#include <winnt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NtCurrentProcess() ((HANDLE) ~(ULONG_PTR)0)
#define NtCurrentThread() ((HANDLE) ~(ULONG_PTR)1)

#define WOW64_TLS_WOW64INFO 10
#define WOW64_TLS_MAX_NUMBER 19

#define WOW64_CPUFLAGS_SOFTWARE 0x02

#define STATUS_EMULATION_SYSCALL ((NTSTATUS)0x40000039)

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

typedef struct {
  ULONG version;
  ULONG unknown1[3];
  ULONG64 unknown2;
  ULONG64 pLdrInitializeThunk;
  ULONG64 pKiUserExceptionDispatcher;
  ULONG64 pKiUserApcDispatcher;
  ULONG64 pKiUserCallbackDispatcher;
  ULONG64 pRtlUserThreadStart;
  ULONG64 pRtlpQueryProcessDebugInformationRemote;
  ULONG64 ntdll_handle;
  ULONG64 pLdrSystemDllInitBlock;
  ULONG64 pRtlpFreezeTimeBias;
} SYSTEM_DLL_INIT_BLOCK;

typedef struct _UNICODE_STRING64 {
  USHORT Length;
  USHORT MaximumLength;
  ULONG64 Buffer;
} UNICODE_STRING64;

typedef struct _CURDIR64 {
  UNICODE_STRING64 DosPath;
  ULONG64 Handle;
} CURDIR64;

typedef struct RTL_DRIVE_LETTER_CURDIR64 {
  USHORT Flags;
  USHORT Length;
  ULONG TimeStamp;
  UNICODE_STRING64 DosPath;
} RTL_DRIVE_LETTER_CURDIR64;

typedef struct _RTL_USER_PROCESS_PARAMETERS64 {
  ULONG AllocationSize;
  ULONG Size;
  ULONG Flags;
  ULONG DebugFlags;
  ULONG64 ConsoleHandle;
  ULONG ConsoleFlags;
  ULONG64 hStdInput;
  ULONG64 hStdOutput;
  ULONG64 hStdError;
  CURDIR64 CurrentDirectory;
  UNICODE_STRING64 DllPath;
  UNICODE_STRING64 ImagePathName;
  UNICODE_STRING64 CommandLine;
  ULONG64 Environment;
  ULONG dwX;
  ULONG dwY;
  ULONG dwXSize;
  ULONG dwYSize;
  ULONG dwXCountChars;
  ULONG dwYCountChars;
  ULONG dwFillAttribute;
  ULONG dwFlags;
  ULONG wShowWindow;
  UNICODE_STRING64 WindowTitle;
  UNICODE_STRING64 Desktop;
  UNICODE_STRING64 ShellInfo;
  UNICODE_STRING64 RuntimeInfo;
  RTL_DRIVE_LETTER_CURDIR64 DLCurrentDirectory[0x20];
  ULONG64 EnvironmentSize;
  ULONG64 EnvironmentVersion;
  ULONG64 PackageDependencyData;
  ULONG ProcessGroupId;
  ULONG LoaderThreads;
} RTL_USER_PROCESS_PARAMETERS64;
typedef struct tagRTL_BITMAP {
  ULONG SizeOfBitMap; /* Number of bits in the bitmap */
  PULONG Buffer;      /* Bitmap data, assumed sized to a DWORD boundary */
} RTL_BITMAP, *PRTL_BITMAP;

typedef const RTL_BITMAP* PCRTL_BITMAP;


typedef struct __PEB {                    /* win32/win64 */
  BOOLEAN InheritedAddressSpace;          /* 000/000 */
  BOOLEAN ReadImageFileExecOptions;       /* 001/001 */
  BOOLEAN BeingDebugged;                  /* 002/002 */
  UCHAR ImageUsedLargePages          : 1; /* 003/003 */
  UCHAR IsProtectedProcess           : 1;
  UCHAR IsImageDynamicallyRelocated  : 1;
  UCHAR SkipPatchingUser32Forwarders : 1;
  UCHAR IsPackagedProcess            : 1;
  UCHAR IsAppContainer               : 1;
  UCHAR IsProtectedProcessLight      : 1;
  UCHAR IsLongPathAwareProcess       : 1;
  HANDLE Mutant;                                    /* 004/008 */
  HMODULE ImageBaseAddress;                         /* 008/010 */
  PPEB_LDR_DATA LdrData;                            /* 00c/018 */
  RTL_USER_PROCESS_PARAMETERS64* ProcessParameters; /* 010/020 */
  PVOID SubSystemData;                              /* 014/028 */
  HANDLE ProcessHeap;                               /* 018/030 */
  PRTL_CRITICAL_SECTION FastPebLock;                /* 01c/038 */
  PVOID AtlThunkSListPtr;                           /* 020/040 */
  PVOID IFEOKey;                                    /* 024/048 */
  ULONG ProcessInJob               : 1;             /* 028/050 */
  ULONG ProcessInitializing        : 1;
  ULONG ProcessUsingVEH            : 1;
  ULONG ProcessUsingVCH            : 1;
  ULONG ProcessUsingFTH            : 1;
  ULONG ProcessPreviouslyThrottled : 1;
  ULONG ProcessCurrentlyThrottled  : 1;
  ULONG ProcessImagesHotPatched    : 1;
  ULONG ReservedBits0              : 24;
  void* KernelCallbackTable;             /* 02c/058 */
  ULONG Reserved;                        /* 030/060 */
  ULONG AtlThunkSListPtr32;              /* 034/064 */
  PVOID ApiSetMap;                       /* 038/068 */
  ULONG TlsExpansionCounter;             /* 03c/070 */
  PRTL_BITMAP TlsBitmap;                 /* 040/078 */
  ULONG TlsBitmapBits[2];                /* 044/080 */
  PVOID ReadOnlySharedMemoryBase;        /* 04c/088 */
  PVOID SharedData;                      /* 050/090 */
  PVOID* ReadOnlyStaticServerData;       /* 054/098 */
  PVOID AnsiCodePageData;                /* 058/0a0 */
  PVOID OemCodePageData;                 /* 05c/0a8 */
  PVOID UnicodeCaseTableData;            /* 060/0b0 */
  ULONG NumberOfProcessors;              /* 064/0b8 */
  ULONG NtGlobalFlag;                    /* 068/0bc */
  LARGE_INTEGER CriticalSectionTimeout;  /* 070/0c0 */
  SIZE_T HeapSegmentReserve;             /* 078/0c8 */
  SIZE_T HeapSegmentCommit;              /* 07c/0d0 */
  SIZE_T HeapDeCommitTotalFreeThreshold; /* 080/0d8 */
  SIZE_T HeapDeCommitFreeBlockThreshold; /* 084/0e0 */
  ULONG NumberOfHeaps;                   /* 088/0e8 */
  ULONG MaximumNumberOfHeaps;            /* 08c/0ec */
  PVOID* ProcessHeaps;                   /* 090/0f0 */
  PVOID GdiSharedHandleTable;            /* 094/0f8 */
  PVOID ProcessStarterHelper;            /* 098/100 */
  PVOID GdiDCAttributeList;              /* 09c/108 */
  PVOID LoaderLock;                      /* 0a0/110 */
  ULONG OSMajorVersion;                  /* 0a4/118 */
  ULONG OSMinorVersion;                  /* 0a8/11c */
  ULONG OSBuildNumber;                   /* 0ac/120 */
  ULONG OSPlatformId;                    /* 0b0/124 */
  ULONG ImageSubSystem;                  /* 0b4/128 */
  ULONG ImageSubSystemMajorVersion;      /* 0b8/12c */
  ULONG ImageSubSystemMinorVersion;      /* 0bc/130 */
  KAFFINITY ActiveProcessAffinityMask;   /* 0c0/138 */
#ifdef _WIN64
  ULONG GdiHandleBuffer[60]; /*    /140 */
#else
  ULONG GdiHandleBuffer[34]; /* 0c4/    */
#endif
  PVOID PostProcessInitRoutine;      /* 14c/230 */
  PRTL_BITMAP TlsExpansionBitmap;    /* 150/238 */
  ULONG TlsExpansionBitmapBits[32];  /* 154/240 */
  ULONG SessionId;                   /* 1d4/2c0 */
  ULARGE_INTEGER AppCompatFlags;     /* 1d8/2c8 */
  ULARGE_INTEGER AppCompatFlagsUser; /* 1e0/2d0 */
  PVOID ShimData;                    /* 1e8/2d8 */
  PVOID AppCompatInfo;               /* 1ec/2e0 */
  UNICODE_STRING64 CSDVersion;       /* 1f0/2e8 */
  PVOID ActivationContextData;       /* 1f8/2f8 */
  PVOID ProcessAssemblyStorageMap;   /* 1fc/300 */
  PVOID SystemDefaultActivationData; /* 200/308 */
  PVOID SystemAssemblyStorageMap;    /* 204/310 */
  SIZE_T MinimumStackCommit;         /* 208/318 */
  PVOID* FlsCallback;                /* 20c/320 */
  LIST_ENTRY FlsListHead;            /* 210/328 */
  PRTL_BITMAP FlsBitmap;             /* 218/338 */
  ULONG FlsBitmapBits[4];            /* 21c/340 */
  ULONG FlsHighIndex;                /* 22c/350 */
  PVOID WerRegistrationData;         /* 230/358 */
  PVOID WerShipAssertPtr;            /* 234/360 */
  PVOID EcCodeBitMap;                /* 238/368 */
  PVOID pImageHeaderHash;            /* 23c/370 */
  ULONG HeapTracingEnabled      : 1; /* 240/378 */
  ULONG CritSecTracingEnabled   : 1;
  ULONG LibLoaderTracingEnabled : 1;
  ULONG SpareTracingBits        : 29;
  ULONGLONG CsrServerReadOnlySharedMemoryBase;  /* 248/380 */
  ULONG TppWorkerpListLock;                     /* 250/388 */
  LIST_ENTRY TppWorkerpList;                    /* 254/390 */
  PVOID WaitOnAddressHashTable[0x80];           /* 25c/3a0 */
  PVOID TelemetryCoverageHeader;                /* 45c/7a0 */
  ULONG CloudFileFlags;                         /* 460/7a8 */
  ULONG CloudFileDiagFlags;                     /* 464/7ac */
  CHAR PlaceholderCompatibilityMode;            /* 468/7b0 */
  CHAR PlaceholderCompatibilityModeReserved[7]; /* 469/7b1 */
  PVOID LeapSecondData;                         /* 470/7b8 */
  ULONG LeapSecondFlags;                        /* 474/7c0 */
  ULONG NtGlobalFlag2;                          /* 478/7c4 */
} __PEB, *__PPEB;

typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME {
  struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME* Previous;
  struct _ACTIVATION_CONTEXT* ActivationContext;
  ULONG Flags;
} RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

typedef struct _ACTIVATION_CONTEXT_STACK {
  RTL_ACTIVATION_CONTEXT_STACK_FRAME* ActiveFrame;
  LIST_ENTRY FrameListCache;
  ULONG Flags;
  ULONG NextCookieSequenceNumber;
  ULONG_PTR StackId;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;
typedef struct _GDI_TEB_BATCH {
  ULONG Offset;
  HANDLE HDC;
  ULONG Buffer[0x136];
} GDI_TEB_BATCH;
typedef struct __TEB {                          /* win32/win64 */
  NT_TIB Tib;                                   /* 000/0000 */
  PVOID EnvironmentPointer;                     /* 01c/0038 */
  CLIENT_ID ClientId;                           /* 020/0040 */
  PVOID ActiveRpcHandle;                        /* 028/0050 */
  PVOID ThreadLocalStoragePointer;              /* 02c/0058 */
  __PPEB Peb;                                   /* 030/0060 */
  ULONG LastErrorValue;                         /* 034/0068 */
  ULONG CountOfOwnedCriticalSections;           /* 038/006c */
  PVOID CsrClientThread;                        /* 03c/0070 */
  PVOID Win32ThreadInfo;                        /* 040/0078 */
  ULONG User32Reserved[26];                     /* 044/0080 */
  ULONG UserReserved[5];                        /* 0ac/00e8 */
  PVOID WOW32Reserved;                          /* 0c0/0100 */
  ULONG CurrentLocale;                          /* 0c4/0108 */
  ULONG FpSoftwareStatusRegister;               /* 0c8/010c */
  PVOID ReservedForDebuggerInstrumentation[16]; /* 0cc/0110 */
#ifdef _WIN64
  PVOID SystemReserved1[30]; /*    /0190 */
#else
  PVOID SystemReserved1[26]; /* 10c/     used for krnl386 private data in Wine */
#endif
  char PlaceholderCompatibilityMode;                       /* 174/0280 */
  BOOLEAN PlaceholderHydrationAlwaysExplicit;              /* 175/0281 */
  char PlaceholderReserved[10];                            /* 176/0282 */
  DWORD ProxiedProcessId;                                  /* 180/028c */
  ACTIVATION_CONTEXT_STACK ActivationContextStack;         /* 184/0290 */
  UCHAR WorkingOnBehalfOfTicket[8];                        /* 19c/02b8 */
  LONG ExceptionCode;                                      /* 1a4/02c0 */
  ACTIVATION_CONTEXT_STACK* ActivationContextStackPointer; /* 1a8/02c8 */
  ULONG_PTR InstrumentationCallbackSp;                     /* 1ac/02d0 */
  ULONG_PTR InstrumentationCallbackPreviousPc;             /* 1b0/02d8 */
  ULONG_PTR InstrumentationCallbackPreviousSp;             /* 1b4/02e0 */
#ifdef _WIN64
  ULONG TxFsContext;                       /*    /02e8 */
  BOOLEAN InstrumentationCallbackDisabled; /*    /02ec */
  BOOLEAN UnalignedLoadStoreExceptions;    /*    /02ed */
#else
  BOOLEAN InstrumentationCallbackDisabled; /* 1b8/     */
  BYTE SpareBytes1[23];                    /* 1b9/     */
  ULONG TxFsContext;                       /* 1d0/     */
#endif
  GDI_TEB_BATCH GdiTebBatch;          /* 1d4/02f0 used for ntdll private data in Wine */
  CLIENT_ID RealClientId;             /* 6b4/07d8 */
  HANDLE GdiCachedProcessHandle;      /* 6bc/07e8 */
  ULONG GdiClientPID;                 /* 6c0/07f0 */
  ULONG GdiClientTID;                 /* 6c4/07f4 */
  PVOID GdiThreadLocaleInfo;          /* 6c8/07f8 */
  ULONG_PTR Win32ClientInfo[62];      /* 6cc/0800 used for user32 private data in Wine */
  PVOID glDispatchTable[233];         /* 7c4/09f0 */
  PVOID glReserved1[29];              /* b68/1138 */
  PVOID glReserved2;                  /* bdc/1220 */
  PVOID glSectionInfo;                /* be0/1228 */
  PVOID glSection;                    /* be4/1230 */
  PVOID glTable;                      /* be8/1238 */
  PVOID glCurrentRC;                  /* bec/1240 */
  PVOID glContext;                    /* bf0/1248 */
  ULONG LastStatusValue;              /* bf4/1250 */
  UNICODE_STRING StaticUnicodeString; /* bf8/1258 */
  WCHAR StaticUnicodeBuffer[261];     /* c00/1268 */
  PVOID DeallocationStack;            /* e0c/1478 */
  PVOID TlsSlots[64];                 /* e10/1480 */
  LIST_ENTRY TlsLinks;                /* f10/1680 */
  PVOID Vdm;                          /* f18/1690 */
  PVOID ReservedForNtRpc;             /* f1c/1698 */
  PVOID DbgSsReserved[2];             /* f20/16a0 */
  ULONG HardErrorMode;                /* f28/16b0 */
#ifdef _WIN64
  PVOID Instrumentation[11]; /*    /16b8 */
#else
  PVOID Instrumentation[9]; /* f2c/ */
#endif
  GUID ActivityId;                   /* f50/1710 */
  PVOID SubProcessTag;               /* f60/1720 */
  PVOID PerflibData;                 /* f64/1728 */
  PVOID EtwTraceData;                /* f68/1730 */
  PVOID WinSockData;                 /* f6c/1738 */
  ULONG GdiBatchCount;               /* f70/1740 */
  ULONG IdealProcessorValue;         /* f74/1744 */
  ULONG GuaranteedStackBytes;        /* f78/1748 */
  PVOID ReservedForPerf;             /* f7c/1750 */
  PVOID ReservedForOle;              /* f80/1758 */
  ULONG WaitingOnLoaderLock;         /* f84/1760 */
  PVOID SavedPriorityState;          /* f88/1768 */
  ULONG_PTR ReservedForCodeCoverage; /* f8c/1770 */
  PVOID ThreadPoolData;              /* f90/1778 */
  PVOID* TlsExpansionSlots;          /* f94/1780 */
#ifdef _WIN64
  union {
    PVOID DeallocationBStore; /*    /1788 */
#ifdef _M_ARM_64EC
    CHPE_V2_CPU_AREA_INFO* ChpeV2CpuAreaInfo; /*    /1788 */
#endif
  } DUMMYUNIONNAME;
  PVOID BStoreLimit; /*    /1790 */
#endif
  ULONG MuiGeneration;            /* f98/1798 */
  ULONG IsImpersonating;          /* f9c/179c */
  PVOID NlsCache;                 /* fa0/17a0 */
  PVOID ShimData;                 /* fa4/17a8 */
  ULONG HeapVirtualAffinity;      /* fa8/17b0 */
  PVOID CurrentTransactionHandle; /* fac/17b8 */
  PVOID ActiveFrame;              /* fb0/17c0 */
  PVOID FlsSlots;                 /* fb4/17c8 */
  PVOID PreferredLanguages;       /* fb8/17d0 */
  PVOID UserPrefLanguages;        /* fbc/17d8 */
  PVOID MergedPrefLanguages;      /* fc0/17e0 */
  ULONG MuiImpersonation;         /* fc4/17e8 */
  USHORT CrossTebFlags;           /* fc8/17ec */
  USHORT SameTebFlags;            /* fca/17ee */
  PVOID TxnScopeEnterCallback;    /* fcc/17f0 */
  PVOID TxnScopeExitCallback;     /* fd0/17f8 */
  PVOID TxnScopeContext;          /* fd4/1800 */
  ULONG LockCount;                /* fd8/1808 */
  LONG WowTebOffset;              /* fdc/180c */
  PVOID ResourceRetValue;         /* fe0/1810 */
  PVOID ReservedForWdf;           /* fe4/1818 */
  ULONGLONG ReservedForCrt;       /* fe8/1820 */
  GUID EffectiveContainerId;      /* ff0/1828 */
} __TEB, *__PTEB;

typedef struct _WOW64INFO {
  ULONG NativeSystemPageSize;
  ULONG CpuFlags;
  ULONG Wow64ExecuteFlags;
  ULONG unknown;
  ULONGLONG SectionHandle;
  ULONGLONG CrossProcessWorkList;
  USHORT NativeMachineType;
  USHORT EmulatedMachineType;
} WOW64INFO;

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

typedef enum _SECTION_INHERIT {
  ViewShare = 1,
  ViewUnmap = 2,
} SECTION_INHERIT;

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
  MemoryFexStatsShm = 2000,
} MEMORY_INFORMATION_CLASS;

#define SystemEmulationBasicInformation (SYSTEM_INFORMATION_CLASS)62

#define ProcessFexHardwareTso (PROCESSINFOCLASS)2000

typedef enum _KEY_VALUE_INFORMATION_CLASS {
  KeyValueBasicInformation,
  KeyValueFullInformation,
  KeyValuePartialInformation,
  KeyValueFullInformationAlign64,
  KeyValuePartialInformationAlign64,
  KeyValueLayerInformation,
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
  ULONG TitleIndex;
  ULONG Type;
  ULONG DataLength;
  UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _MEMORY_FEX_STATS_SHM_INFORMATION {
  void* shm_base;
  DWORD map_size;
  DWORD max_size;
} MEMORY_FEX_STATS_SHM_INFORMATION, *PMEMORY_FEX_STATS_SHM_INFORMATION;

typedef struct _MEMORY_SECTION_NAME {
  UNICODE_STRING SectionFileName;
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

NTSTATUS WINAPIV DbgPrint(LPCSTR fmt, ...);
NTSTATUS WINAPI LdrDisableThreadCalloutsForDll(HMODULE);
NTSTATUS WINAPI LdrGetDllFullName(HMODULE, UNICODE_STRING*);
NTSTATUS WINAPI LdrGetDllHandle(LPCWSTR, ULONG, const UNICODE_STRING*, HMODULE*);
NTSTATUS WINAPI LdrGetProcedureAddress(HMODULE, const ANSI_STRING*, ULONG, void**);
NTSTATUS WINAPI NtAllocateVirtualMemoryEx(HANDLE, PVOID*, SIZE_T*, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
NTSTATUS WINAPI NtAllocateVirtualMemory(HANDLE, PVOID*, ULONG_PTR, SIZE_T*, ULONG, ULONG);
NTSTATUS WINAPI NtContinue(PCONTEXT, BOOLEAN);
NTSTATUS WINAPI NtCreateSection(HANDLE*, ACCESS_MASK, const OBJECT_ATTRIBUTES*, const LARGE_INTEGER*, ULONG, ULONG, HANDLE);
NTSTATUS WINAPI NtDuplicateObject(HANDLE, HANDLE, HANDLE, PHANDLE, ACCESS_MASK, ULONG, ULONG);
NTSTATUS WINAPI NtFlushInstructionCache(HANDLE, LPCVOID, SIZE_T);
NTSTATUS WINAPI NtFreeVirtualMemory(HANDLE, PVOID*, SIZE_T*, ULONG);
NTSTATUS WINAPI NtGetContextThread(HANDLE, CONTEXT*);
ULONG WINAPI NtGetCurrentProcessorNumber(void);
NTSYSAPI NTSTATUS WINAPI NtMapViewOfSection(HANDLE, HANDLE, PVOID*, ULONG_PTR, SIZE_T, const LARGE_INTEGER*, SIZE_T*, SECTION_INHERIT, ULONG, ULONG);
NTSTATUS WINAPI NtOpenKeyEx(PHANDLE, ACCESS_MASK, const OBJECT_ATTRIBUTES*, ULONG);
NTSTATUS WINAPI NtProtectVirtualMemory(HANDLE, PVOID*, SIZE_T*, ULONG, ULONG*);
NTSTATUS WINAPI NtQueryAttributesFile(const OBJECT_ATTRIBUTES*, FILE_BASIC_INFORMATION*);
NTSTATUS WINAPI NtQueryValueKey(HANDLE, const UNICODE_STRING*, KEY_VALUE_INFORMATION_CLASS, void*, DWORD, DWORD*);
NTSTATUS WINAPI NtQueryVirtualMemory(HANDLE, LPCVOID, MEMORY_INFORMATION_CLASS, PVOID, SIZE_T, SIZE_T*);
NTSTATUS WINAPI NtReadFile(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
NTSTATUS WINAPI NtSetContextThread(HANDLE, const CONTEXT*);
NTSTATUS WINAPI NtSuspendThread(HANDLE, PULONG);
NTSTATUS WINAPI NtTerminateProcess(HANDLE, LONG);
NTSTATUS WINAPI NtWriteFile(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, const void*, ULONG, PLARGE_INTEGER, PULONG);
void WINAPI ProcessPendingCrossProcessEmulatorWork();
void WINAPI RtlAcquirePebLock(void);
void WINAPI RtlAcquireSRWLockExclusive(RTL_SRWLOCK*);
void WINAPI RtlClearBits(PRTL_BITMAP, ULONG, ULONG);
NTSTATUS WINAPI RtlDeleteCriticalSection(RTL_CRITICAL_SECTION*);
NTSTATUS WINAPI RtlEnterCriticalSection(RTL_CRITICAL_SECTION*);
ULONG WINAPI RtlFindClearBitsAndSet(PRTL_BITMAP, ULONG, ULONG);
ULONG WINAPI RtlGetCurrentDirectory_U(ULONG, LPWSTR);
PIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE);
PVOID WINAPI RtlImageDirectoryEntryToData(HMODULE, BOOL, WORD, ULONG*);
void WINAPI RtlInitializeConditionVariable(RTL_CONDITION_VARIABLE*);
NTSTATUS WINAPI RtlInitializeCriticalSection(RTL_CRITICAL_SECTION*);
void WINAPI RtlInitializeSRWLock(RTL_SRWLOCK*);
NTSTATUS WINAPI RtlLeaveCriticalSection(RTL_CRITICAL_SECTION*);
void* WINAPI RtlLocateExtendedFeature(CONTEXT_EX*, ULONG, ULONG*);
NTSTATUS WINAPI RtlMultiByteToUnicodeN(LPWSTR, DWORD, LPDWORD, LPCSTR, DWORD);
NTSTATUS WINAPI RtlMultiByteToUnicodeSize(DWORD*, LPCSTR, ULONG);
BOOL WINAPI RtlQueryPerformanceCounter(LARGE_INTEGER*);
BOOL WINAPI RtlQueryPerformanceFrequency(LARGE_INTEGER*);
void WINAPI RtlReleasePebLock(void);
void WINAPI RtlReleaseSRWLockExclusive(RTL_SRWLOCK*);
NTSTATUS WINAPI RtlSleepConditionVariableSRW(RTL_CONDITION_VARIABLE*, RTL_SRWLOCK*, const LARGE_INTEGER*, ULONG);
BOOLEAN WINAPI RtlTryAcquireSRWLockExclusive(RTL_SRWLOCK*);
BOOL WINAPI RtlTryEnterCriticalSection(RTL_CRITICAL_SECTION*);
NTSTATUS WINAPI RtlUnicodeToMultiByteN(LPSTR, DWORD, LPDWORD, LPCWSTR, DWORD);
void WINAPI RtlWakeAllConditionVariable(RTL_CONDITION_VARIABLE*);
void WINAPI RtlWakeConditionVariable(RTL_CONDITION_VARIABLE*);
NTSTATUS WINAPI RtlWow64GetCurrentCpuArea(USHORT*, void**, void**);
NTSTATUS WINAPI RtlWow64GetThreadContext(HANDLE, WOW64_CONTEXT*);
NTSTATUS WINAPI RtlWow64SetThreadContext(HANDLE, const WOW64_CONTEXT*);
void WINAPI Wow64ProcessPendingCrossProcessItems(void);
NTSTATUS WINAPI Wow64SystemServiceEx(UINT, UINT*);
NTSTATUS WINAPI RtlWow64SuspendThread(HANDLE, ULONG*);

#ifdef __cplusplus
}
#endif
