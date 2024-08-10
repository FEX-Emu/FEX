// SPDX-License-Identifier: MIT
#pragma once

#include <windef.h>
#include <ntstatus.h>
#include <winternl.h>

extern "C" {
void STDMETHODCALLTYPE BTCpuProcessInit();
void STDMETHODCALLTYPE BTCpuProcessTerm(HANDLE Handle, BOOL After, ULONG Status);
void STDMETHODCALLTYPE BTCpuThreadInit();
void STDMETHODCALLTYPE BTCpuThreadTerm(HANDLE Thread, LONG ExitCode);
void STDMETHODCALLTYPE* BTCpuGetBopCode();
void STDMETHODCALLTYPE* __wine_get_unix_opcode();
NTSTATUS STDMETHODCALLTYPE BTCpuGetContext(HANDLE Thread, HANDLE Process, void* Unknown, WOW64_CONTEXT* Context);
NTSTATUS STDMETHODCALLTYPE BTCpuSetContext(HANDLE Thread, HANDLE Process, void* Unknown, WOW64_CONTEXT* Context);
void STDMETHODCALLTYPE BTCpuSimulate();
NTSTATUS STDMETHODCALLTYPE BTCpuSuspendLocalThread(HANDLE Thread, ULONG* Count);
NTSTATUS STDMETHODCALLTYPE BTCpuResetToConsistentState(EXCEPTION_POINTERS* Ptrs);
void STDMETHODCALLTYPE BTCpuFlushInstructionCache2(const void* Address, SIZE_T Size);
void STDMETHODCALLTYPE BTCpuFlushInstructionCacheHeavy(const void* Address, SIZE_T Size);
void STDMETHODCALLTYPE BTCpuNotifyMemoryAlloc(void* Address, SIZE_T Size, ULONG Type, ULONG Prot, BOOL After, ULONG Status);
void STDMETHODCALLTYPE BTCpuNotifyMemoryProtect(void* Address, SIZE_T Size, ULONG NewProt, BOOL After, ULONG Status);
void STDMETHODCALLTYPE BTCpuNotifyMemoryDirty(void* Address, SIZE_T Size);
void STDMETHODCALLTYPE BTCpuNotifyMemoryFree(void* Address, SIZE_T Size, ULONG FreeType, BOOL After, ULONG Status);
NTSTATUS STDMETHODCALLTYPE BTCpuNotifyMapViewOfSection(void* Unk1, void* Address, void* Unk2, SIZE_T Size, ULONG AllocType, ULONG Prot);
void STDMETHODCALLTYPE BTCpuNotifyUnmapViewOfSection(void* Address, BOOL After, ULONG Status);
void STDMETHODCALLTYPE BTCpuNotifyReadFile(HANDLE Handle, void* Address, SIZE_T Size, BOOL After, NTSTATUS Status);
BOOLEAN STDMETHODCALLTYPE BTCpuIsProcessorFeaturePresent(UINT Feature);
void STDMETHODCALLTYPE BTCpuUpdateProcessorInformation(SYSTEM_CPU_INFORMATION* Info);
}
