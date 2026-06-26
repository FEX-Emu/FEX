// SPDX-License-Identifier: MIT
#include "common/Host.h"
#include "cuda_defines.h"

#include <stdio.h>
#include <dlfcn.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include "thunkgen_host_libcuda.inl"

#define FEXFN_IMPL(fn) fexfn_impl_libcuda_##fn
#define LDR_PTR(fn) fexldr_ptr_libcuda_##fn

struct override_entry {
  std::string_view name;
  void* ptr;
};

const static std::array<override_entry, 2> proc_override = {
  {{"cuCtxCreate_v2", (void*)FEXFN_IMPL(cuCtxCreate_v2)}, {"cuGetExportTable", (void*)FEXFN_IMPL(cuGetExportTable)}}};

static CUresult FEXFN_IMPL(cuGetProcAddress_v2)(const char* symbol, guest_layout<void**> pfn, int cudaVersion, cuuint64_t flags,
                                                CUdriverProcAddressQueryResult* symbolStatus) {
  host_layout<void*> host_data {};
  void* ptr {};
  CUresult ret {};
  for (auto& over : proc_override) {
    if (symbol == over.name) {
      ptr = over.ptr;
      if (symbolStatus) {
        *symbolStatus = (CUdriverProcAddressQueryResult)0; // CU_GET_PROC_ADDRESS_SUCCESS
      }
      ret = (CUresult)0; // CUDA_SUCCESS
      break;
    }
  }

  if (!ptr) {
    ret = LDR_PTR(cuGetProcAddress_v2)(symbol, &ptr, cudaVersion, flags, symbolStatus);
  }

  host_data.data = ptr;
  *pfn.get_pointer() = to_guest(host_data);
  return ret;
}

CUresult FEXFN_IMPL(cuCtxCreate_v2)(guest_layout<CUcontext*> pctx, unsigned int flags, CUdevice dev) {
  host_layout<CUcontext> host_data {};
  CUcontext ctx;
  auto ret = LDR_PTR(cuCtxCreate_v2)(&ctx, flags, dev);
  host_data.data = ctx;
  *pctx.get_pointer() = to_guest(host_data);
  return ret;
}

CUresult FEXFN_IMPL(cuGetExportTable)(guest_layout<const void**> ppExportTable, const CUuuid* pExportTableId) {
  // This function returns a pointer to an driver internal export table that is undocumented publicly.
  //
  // Some documentation about the UUIDs that has been reversed.
  // https://github.com/vosen/ZLUDA/blob/1b9ba2b2333746c5e2b05a2bf24fa6ec3828dcdf/zluda_dark_api/src/lib.rs#L197
  //
  // These two UUIDs are required by the static cuda runtime at startup. Might require more but it currently halts without these
  // implemented. UUID: [0x6b, 0xd5, 0xfb, 0x6c, 0x5b, 0xf4, 0xe7, 0x4a, 0x89, 0x87, 0xd9, 0x39, 0x12, 0xfd, 0x9d, 0xf9]
  // - cudart interface (0x68 bytes?)
  // UUID: [0xa0, 0x94, 0x79, 0x8c, 0x2e, 0x74, 0x2e, 0x74, 0x93, 0xf2, 0x08, 0x00, 0x20, 0x0c, 0x0a, 0x66]
  // - Tools runtime callback hooks (0x38 bytes?)

  std::string uuid {};
  for (size_t i = 0; i < sizeof(CUuuid); ++i) {
    const bool last = (i + 1) == sizeof(CUuuid);
    char tmp[8];
    auto size = snprintf(tmp, 8, "0x%02x%s", pExportTableId->_0[i], last ? "" : ", ");
    uuid += std::string_view(tmp, size);
  }
  fprintf(stderr, "cuGetExportTable not implemented\n");
  fprintf(stderr, "UUID: %s\n", uuid.c_str());
  return (CUresult)4; // CUDA_ERROR_DEINITIALIZED
}

EXPORTS(libcuda)
