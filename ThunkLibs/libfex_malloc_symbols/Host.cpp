/*
$info$
tags: thunklibs|fex_malloc_symbols
desc: Allows FEX to export allocation symbols
$end_info$
*/

#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <memory.h>

#include "common/Host.h"
#include <dlfcn.h>

#include "../libfex_malloc/Types.h"

extern "C" {
// FEX allocation routines
MallocPtr FEX_Malloc_Ptr = (MallocPtr)0x4142434445464748ULL;
FreePtr FEX_Free_Ptr = (FreePtr)0x4142434445464748ULL;
CallocPtr FEX_Calloc_Ptr = (CallocPtr)0x4142434445464748ULL;
MemalignPtr FEX_Memalign_Ptr = (MemalignPtr)0x4142434445464748ULL;
ReallocPtr FEX_Realloc_Ptr = (ReallocPtr)0x4142434445464748ULL;
VallocPtr FEX_Valloc_Ptr = (VallocPtr)0x4142434445464748ULL;
PosixMemalignPtr FEX_PosixMemalign_Ptr = (PosixMemalignPtr)0x4142434445464748ULL;
AlignedAllocPtr FEX_AlignedAlloc_Ptr = (AlignedAllocPtr)0x4142434445464748ULL;
MallocUsablePtr FEX_MallocUsable_Ptr = (MallocUsablePtr)0x4142434445464748ULL;
}
