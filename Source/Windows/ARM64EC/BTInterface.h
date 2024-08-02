// SPDX-License-Identifier: MIT
#pragma once

#include <windef.h>
#include <ntstatus.h>
#include <winternl.h>

extern "C" {
NTSTATUS STDMETHODCALLTYPE ProcessInit();
void STDMETHODCALLTYPE ProcessTerm(HANDLE Handle, BOOL After, NTSTATUS Status);
NTSTATUS STDMETHODCALLTYPE ThreadInit();
NTSTATUS STDMETHODCALLTYPE ThreadTerm(HANDLE Thread, LONG ExitCode);
NTSTATUS STDMETHODCALLTYPE ResetToConsistentState(EXCEPTION_RECORD* Exception, CONTEXT* GuestContext, ARM64_NT_CONTEXT* NativeContext);
void STDMETHODCALLTYPE NotifyMemoryAlloc(void* Address, SIZE_T Size, ULONG Type, ULONG Prot, BOOL After, NTSTATUS Status);
void STDMETHODCALLTYPE NotifyMemoryFree(void* Address, SIZE_T Size, ULONG FreeType, BOOL After, NTSTATUS Status);
void STDMETHODCALLTYPE NotifyMemoryProtect(void* Address, SIZE_T Size, ULONG NewProt, BOOL After, NTSTATUS Status);
NTSTATUS STDMETHODCALLTYPE NotifyMapViewOfSection(void* Unk1, void* Address, void* Unk2, SIZE_T Size, ULONG AllocType, ULONG Prot);
void STDMETHODCALLTYPE NotifyUnmapViewOfSection(void* Address, BOOL After, NTSTATUS Status);
void STDMETHODCALLTYPE FlushInstructionCacheHeavy(const void* Address, SIZE_T Size);
void STDMETHODCALLTYPE BTCpu64FlushInstructionCache(const void* Address, SIZE_T Size);
void STDMETHODCALLTYPE BTCpu64NotifyMemoryDirty(void* Address, SIZE_T Size);
void STDMETHODCALLTYPE BTCpu64NotifyReadFile(HANDLE Handle, void* Address, SIZE_T Size, BOOL After, NTSTATUS Status);
BOOLEAN STDMETHODCALLTYPE BTCpu64IsProcessorFeaturePresent(UINT Feature);
void STDMETHODCALLTYPE UpdateProcessorInformation(SYSTEM_CPU_INFORMATION* Info);
}
