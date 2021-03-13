#include "Interface/Core/ArchHelpers/Dwarf.h"
#include "Interface/Core/ArchHelpers/DwarfOps.h"
#include "Interface/Core/ArchHelpers/StateReg.h"
#include "Interface/Core/CodeBuffer.h"
#include <FEXCore/Core/CoreState.h>

#include <string>
#include <stdint.h>
#include <utility>
#include <string.h>

#include <unwind.h>
#include <map>


extern "C" {
  // These are somewhat undocumented functions that glibc exposes
  extern void __register_frame(void* fde);
  extern void __deregister_frame(void* fde);
}

namespace FEXCore::CPU {

std::map<uint64_t, std::string> FDE_mapping;

_Unwind_Reason_Code UnwindBacktraceCallback(struct _Unwind_Context* unwind_context, void* state_voidp) {
    uint64_t ip = _Unwind_GetIP(unwind_context);
    uint64_t start = _Unwind_GetRegionStart(unwind_context);
    uint64_t ra = _Unwind_GetGR(unwind_context, 16);

    printf("ip %lx, region %lx, ra %lx\n", ip, start, ra);

    if (FDE_mapping.contains(start)) {
        uint64_t cfa = _Unwind_GetCFA(unwind_context);
        uint64_t ra = _Unwind_GetGR(unwind_context, 16);
        printf("Dynamic FDE %s\ncfa = %lx, ra = %lx\n", FDE_mapping[start].c_str(), cfa, ra);
    }

    return _URC_NO_REASON;
}

void Backtrace() {
    _Unwind_Backtrace(UnwindBacktraceCallback, nullptr);
}



DwarfFrame::DwarfFrame(CodeBuffer *Buffer) {
  EmitDwarfForCodeBuffer(Buffer);

  // Register the FDE with glibc
  // We pass a pointer directly to the FDE, glibc will look backwards for the CIE
  __register_frame(DwarfData.data() + FdeOffset);
}

DwarfFrame::~DwarfFrame() {
  // Make sure we unregister the same pointer
  if (!DwarfData.empty())
    __deregister_frame(DwarfData.data() + FdeOffset);
}


void DwarfFrame::EmitDwarfForCodeBuffer(CodeBuffer *Buffer) {
  // Various emitters
  auto emit_byte = [&] (uint8_t b) { DwarfData.push_back(b); };
  auto emit_short = [&] (uint16_t s) { emit_byte(s & 0xff); emit_byte(s >> 8); };
  auto emit_word = [&] (uint32_t w) { emit_short(w & 0xffff); emit_short(w >> 16); };
  auto emit_u64  = [&] (uint64_t l) { emit_word(l & 0xffffffff); emit_word(l >> 32); };
  auto emit_uLEB128 = [&] (uint64_t num) {
    bool more = true;
    while (more) {
        more = num > 0x7f;
        emit_byte((num & 0x7f) | (more << 7));
        num = num >> 7;
    }
  };
  auto emit_sLEB128 = [&] (int64_t num) {
    bool more = true;
    while (more) {
        uint8_t byte = num & 0x7f;
        bool sign = (byte & 0x40) != 0;
        num = num >> 7;
        more = (num != 0 || sign) && (num != -1 || !sign);
        emit_byte(byte | (more << 7));
    }

  };
  auto emit_string = [&] (std::string str) {
      for (auto &c : str) {
          emit_byte(c);
      }
      emit_byte('\0');
  };

  auto fixup32 = [&] (size_t offset, uint32_t val) {
      DwarfData[offset++] = (val >>  0) & 0xFF;
      DwarfData[offset++] = (val >>  8) & 0xFF;
      DwarfData[offset++] = (val >> 16) & 0xFF;
      DwarfData[offset++] = (val >> 24) & 0xFF;
  };

  // CIE (Common Information Entry)
  // Holds common information that can be shared by multiple FDEs
  emit_word(0x10); // length
  emit_word(0x0); // CIE_ID
  emit_byte(1); // CIE_VERSION
  emit_string("zR"); // augmentation tags (see augmentation data below)
  emit_uLEB128(1); // Code alignment factor
  emit_sLEB128(-8); // Data alignment factor
  emit_byte(16); // Return address register
      // Augmentation data
      emit_uLEB128(1); // z = length
      emit_byte(0x04); // R = DW_EH_PE_absptr | DW_EH_PE_udata8; Pointers are 8 byte absolute

  // Initial instructions
  while (DwarfData.size() & 0x3)  // PADDING
  {
      emit_byte(0x00); // DW_CFA_nop
  }

  FdeOffset = DwarfData.size();

  // FDE (Frame Description Entry (FDE))
  // In theory this describes one function
  // But we are going to abuse things and use one to describe the whole jit code buffer
  emit_word(0); // length (we will backpatch this below)
  emit_word(DwarfData.size()); // relative offset to CIE (but negated)
  emit_u64(reinterpret_cast<uint64_t>(Buffer->Ptr)); // pc start
  emit_u64(Buffer->Size); // pc length
  emit_uLEB128(0); // No augmentation

  // FDE instructions
  // This is a bytecode that libunwind (part of libc) will execute to find the state at
  // any given instruction within the "function"
  //
  // Because our jitted code ALWAYS keeps the current state loaded in r14 (x86_64) or r28 (Arm64)
  // we can treat this kind of like a base pointer
  // The following bytecode will apply to the whole codebuffer
  {
      // Step one, we need to find the CFA (Call Frame)
      // Which is just the fake call frame we setup when entering the dispatcher

      // CFA = (STATE)->ReturningStackLocation
      emit_byte(DW_CFA_def_cfa_expression); // We need a full expression to specify this
          emit_uLEB128(7); // num bytes
          emit_byte(DW_OP_reg(STATE_host)); // load STATE onto stack
          emit_byte(DW_OP_const2u); // load the offset to ReturningStackLocation
          emit_short(offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation));
          emit_byte(DW_OP_plus); // calculate address of ReturningStackLocation
          emit_byte(DW_OP_deref); // read the value of ReturningStackLocation
          emit_byte(DW_OP_nop);

      // That's all we really need. Once the unwinder has found the return address
      // it can continue up the stack and interrogate more call frames

      // Most call frames contain callee saved registers
      // But our special fake callframe doesn't, they are actually stored one frame higher
      // So mark them as undefined
      emit_byte(DW_CFA_undefined); emit_uLEB128(15);
      emit_byte(DW_CFA_undefined); emit_uLEB128(14);
      emit_byte(DW_CFA_undefined); emit_uLEB128(13);
      emit_byte(DW_CFA_undefined); emit_uLEB128(12);
      emit_byte(DW_CFA_undefined); emit_uLEB128(6);
      emit_byte(DW_CFA_undefined); emit_uLEB128(3);


      // Return address can be found at CFE+0
      emit_byte(DW_CFA_offset(16)); emit_uLEB128(0);

      // Advance the program counter to the end of the code buffer so everything above
      // will apply to the whole codebuffer
      emit_byte(DW_CFA_advance_loc4); emit_word(Buffer->Size);

      while (DwarfData.size() & 0x3) { // PADDING
          emit_byte(DW_CFA_nop);
      }
  }

  // Fixup FDE length
  fixup32(FdeOffset, DwarfData.size() - (FdeOffset + 4));

  // Next FDE
  // Specify a length of zero to signal that there are no more FDEs
  emit_word(0); // length
}

}