#include <FEXCore/Core/UContext.h>
#include <FEXCore/Utils/LogManager.h>

#include "CoreDumpService.h"
#include "Unwind_x86_64.h"
#include "ELFMapping.h"
#include "FileMapping.h"
#include <libunwind-x86_64.h>
#include <dwarf.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <list>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace Unwind::x86_64 {
#define FORWARD_DECLARE
#include "Unwind/Unwind_accessors.inl"

  class Unwinder_x86_64 final : public Unwinder {
    public:
      Unwinder_x86_64(CoreDumpService::CoreDumpClass *CoreDump, void *Info, void *Context)
        : Unwinder(CoreDump) {
        siginfo = *(siginfo_t*)Info;
        _context = *(FEXCore::x86_64::mcontext_t*)Context;

        addr = unw_create_addr_space(&accessors, __LITTLE_ENDIAN);

        if (unw_init_remote (&cursor, addr, this) < 0) {
          fmt::print(stderr, "unw_init_remote failed!\n");
          CanBacktrace = false;
        }
      }

      void Backtrace() override;

      void BacktraceHeader() {
        fmt::print(stderr, "Register data\n");
        fmt::print(stderr, "RIP: {:016x}\n", _context.gregs[FEXCore::x86_64::FEX_REG_RIP]);
        fmt::print(stderr, "RAX: {:016x} RBX: {:016x} RCX: {:016x} RDX {:016x}\n",
          _context.gregs[FEXCore::x86_64::FEX_REG_RAX], _context.gregs[FEXCore::x86_64::FEX_REG_RBX],
          _context.gregs[FEXCore::x86_64::FEX_REG_RCX], _context.gregs[FEXCore::x86_64::FEX_REG_RDX]);
        fmt::print(stderr, "RSI: {:016x} RDI: {:016x} RSP: {:016x} RBP {:016x}\n",
          _context.gregs[FEXCore::x86_64::FEX_REG_RSI], _context.gregs[FEXCore::x86_64::FEX_REG_RDI],
          _context.gregs[FEXCore::x86_64::FEX_REG_RSP], _context.gregs[FEXCore::x86_64::FEX_REG_RBP]);
        fmt::print(stderr, "R8:  {:016x} R9:  {:016x} R10: {:016x} R11 {:016x}\n",
          _context.gregs[FEXCore::x86_64::FEX_REG_R8], _context.gregs[FEXCore::x86_64::FEX_REG_R9],
          _context.gregs[FEXCore::x86_64::FEX_REG_R10], _context.gregs[FEXCore::x86_64::FEX_REG_R11]);
        fmt::print(stderr, "R12: {:016x} R13: {:016x} R14: {:016x} R15 {:016x}\n",
          _context.gregs[FEXCore::x86_64::FEX_REG_R12], _context.gregs[FEXCore::x86_64::FEX_REG_R13],
          _context.gregs[FEXCore::x86_64::FEX_REG_R14], _context.gregs[FEXCore::x86_64::FEX_REG_R15]);

        fmt::print(stderr, "\n");
      }

      FEXCore::x86_64::mcontext_t * GetContext() {
        return &_context;
      }

      unw_word_t GetReg(unw_regnum_t regnum) {
        switch (regnum) {
#define GETREG(x) case UNW_X86_64_##x: return _context.gregs[FEXCore::x86_64::FEX_REG_##x]; break
          GETREG(RAX);
          GETREG(RDX);
          GETREG(RCX);
          GETREG(RBX);
          GETREG(RSI);
          GETREG(RDI);
          GETREG(RBP);
          GETREG(RSP);
          GETREG(R8);
          GETREG(R9);
          GETREG(R10);
          GETREG(R11);
          GETREG(R12);
          GETREG(R13);
          GETREG(R14);
          GETREG(R15);
          GETREG(RIP);
#undef GETREG
          default:
            fmt::print(stderr, "Unhandled access regnum: %d", regnum);
        }
        return 0;
      }
      void SetReg(unw_regnum_t regnum, unw_word_t Value) {
        switch (regnum) {
#define GETREG(x) case UNW_X86_64_##x: _context.gregs[FEXCore::x86_64::FEX_REG_##x] = Value; break
          GETREG(RAX);
          GETREG(RDX);
          GETREG(RCX);
          GETREG(RBX);
          GETREG(RSI);
          GETREG(RDI);
          GETREG(RBP);
          GETREG(RSP);
          GETREG(R8);
          GETREG(R9);
          GETREG(R10);
          GETREG(R11);
          GETREG(R12);
          GETREG(R13);
          GETREG(R14);
          GETREG(R15);
          GETREG(RIP);
#undef GETREG
          default:
            fmt::print(stderr, "Unhandled access regnum: %d", regnum);
        }
      }

    private:
      siginfo_t siginfo;
      FEXCore::x86_64::mcontext_t _context;

      unw_cursor_t cursor;
      unw_addr_space_t addr;

      unw_accessors_t accessors {
        .find_proc_info         = UnwinderFuncs::find_proc_info,
        .put_unwind_info        = UnwinderFuncs::put_unwind_info,
        .get_dyn_info_list_addr = UnwinderFuncs::get_dyn_info_list_addr,
        .access_mem             = UnwinderFuncs::access_mem,
        .access_reg             = UnwinderFuncs::access_reg,
        .access_fpreg           = UnwinderFuncs::access_fpreg,
        .resume                 = UnwinderFuncs::resume,
        .get_proc_name          = UnwinderFuncs::get_proc_name,
      };
  };

#define ACCESSOR_IMPL
#define UNWINDER_TYPE Unwinder_x86_64
#include "Unwind/Unwind_accessors.inl"

  Unwinder *Unwind(CoreDumpService::CoreDumpClass *CoreDump, void *Info, void *Context) {
    auto Unwind = new Unwinder_x86_64(CoreDump, Info, Context);
    return Unwind;
  }

}
