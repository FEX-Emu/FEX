#pragma once

#include <windef.h>
#include <ntstatus.h>
#include <winternl.h>

extern "C" {
void STDMETHODCALLTYPE BTCpuProcessInit();
NTSTATUS STDMETHODCALLTYPE BTCpuThreadInit();
NTSTATUS STDMETHODCALLTYPE BTCpuThreadTerm(HANDLE Thread);
void STDMETHODCALLTYPE *BTCpuGetBopCode();
void STDMETHODCALLTYPE *__wine_get_unix_opcode();
NTSTATUS STDMETHODCALLTYPE BTCpuGetContext(HANDLE Thread, HANDLE Process, void *Unknown, WOW64_CONTEXT *Context);
NTSTATUS STDMETHODCALLTYPE BTCpuSetContext(HANDLE Thread, HANDLE Process, void *Unknown, WOW64_CONTEXT *Context);
void STDMETHODCALLTYPE BTCpuSimulate();
NTSTATUS STDMETHODCALLTYPE BTCpuSuspendLocalThread(HANDLE Thread, ULONG *Count);
NTSTATUS STDMETHODCALLTYPE BTCpuResetToConsistentState(EXCEPTION_POINTERS *Ptrs);
void STDMETHODCALLTYPE BTCpuFlushInstructionCache2(const void *Address, SIZE_T Size);
void STDMETHODCALLTYPE BTCpuNotifyMemoryAlloc(void *Address, SIZE_T Size, ULONG Type, ULONG Prot);
void STDMETHODCALLTYPE BTCpuNotifyMemoryProtect(void *Address, SIZE_T Size, ULONG NewProt);
void STDMETHODCALLTYPE BTCpuNotifyMemoryFree(void *Address, SIZE_T Size, ULONG FreeType);
void STDMETHODCALLTYPE BTCpuNotifyUnmapViewOfSection(void *Address, ULONG Flags);
BOOLEAN STDMETHODCALLTYPE BTCpuIsProcessorFeaturePresent(UINT Feature);
BOOLEAN STDMETHODCALLTYPE BTCpuUpdateProcessorInformation(SYSTEM_CPU_INFORMATION *Info);
}
