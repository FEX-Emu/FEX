/*
$info$
tags: backend|riscv64
$end_info$
*/

#include "Interface/Core/JIT/RISCV/JITClass.h"
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::CPU {
using namespace biscuit;
#define DEF_OP(x) void RISCVJITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(CASPair) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(CAS) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(AtomicAdd) {
  auto Op = IROp->C<IR::IROp_AtomicAdd>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the add
      ADDUW(TMP2, TMP2, TMP3);
      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP1, TMP1, -256);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the add
      ADDUW(TMP2, TMP2, TMP3);
      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP1, TMP1, TMP3);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 4: AMOADD_W(Ordering::AQRL, zero, GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOADD_D(Ordering::AQRL, zero, GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicSub) {
  auto Op = IROp->C<IR::IROp_AtomicSub>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      SUB(TMP2, TMP2, TMP3);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP1, TMP1, -256);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      SUB(TMP2, TMP2, TMP3);

      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP1, TMP1, TMP3);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 4:
      NEG(TMP1, GetReg(Op->Value.ID()));
      AMOADD_W(Ordering::AQRL, zero, TMP1, MemSrc);
      break;
    case 8:
      NEG(TMP1, GetReg(Op->Value.ID()));
      AMOADD_D(Ordering::AQRL, zero, TMP1, MemSrc);
      break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicAnd) {
  auto Op = IROp->C<IR::IROp_AtomicAnd>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      AND(TMP2, TMP2, TMP3);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP1, TMP1, -256);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      AND(TMP2, TMP2, TMP3);

      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP1, TMP1, TMP3);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 4: AMOAND_W(Ordering::AQRL, zero, GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOAND_D(Ordering::AQRL, zero, GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicOr) {
  auto Op = IROp->C<IR::IROp_AtomicOr>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      OR(TMP2, TMP2, TMP3);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP1, TMP1, -256);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      OR(TMP2, TMP2, TMP3);

      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP1, TMP1, TMP3);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 4: AMOOR_W(Ordering::AQRL, zero, GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOOR_D(Ordering::AQRL, zero, GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicXor) {
  auto Op = IROp->C<IR::IROp_AtomicXor>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      XOR(TMP2, TMP2, TMP3);
      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP1, TMP1, -256);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      XOR(TMP2, TMP2, TMP3);
      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP1, TMP1, TMP3);

      // Insert result
      OR(TMP1, TMP1, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 4: AMOXOR_W(Ordering::AQRL, zero, GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOXOR_D(Ordering::AQRL, zero, GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicSwap) {
  auto Op = IROp->C<IR::IROp_AtomicSwap>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Mask out 8bit bottom from source
      ANDI(TMP1, TMP1, -256);

      // Insert result
      OR(TMP1, TMP1, TMP3);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP2, 0xFFFF'0000 >> 12);
      AND(TMP1, TMP1, TMP2);

      // Insert result
      OR(TMP1, TMP1, TMP3);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP1, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    case 4: AMOSWAP_W(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOSWAP_D(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicFetchAdd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the add
      ADDUW(TMP2, TMP2, TMP3);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP3, TMP1, -256);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTB(GetReg(Node), TMP1);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the add
      ADDUW(TMP2, TMP2, TMP3);
      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP3, TMP1, TMP3);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      EBREAK();
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTH(GetReg(Node), TMP1);
      break;
    }
    case 4: AMOADD_W(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOADD_D(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicFetchSub) {
  auto Op = IROp->C<IR::IROp_AtomicFetchSub>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      SUB(TMP2, TMP2, TMP3);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP3, TMP1, -256);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTB(GetReg(Node), TMP1);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      SUB(TMP2, TMP2, TMP3);

      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP3, TMP1, TMP3);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTH(GetReg(Node), TMP1);
      break;
    }
    case 4:
      NEG(TMP1, GetReg(Op->Value.ID()));
      AMOADD_W(Ordering::AQRL, GetReg(Node), TMP1, MemSrc);
      break;
    case 8:
      NEG(TMP1, GetReg(Op->Value.ID()));
      AMOADD_D(Ordering::AQRL, GetReg(Node), TMP1, MemSrc);
      break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicFetchAnd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAnd>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      AND(TMP2, TMP2, TMP3);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP3, TMP1, -256);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTB(GetReg(Node), TMP1);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      AND(TMP2, TMP2, TMP3);

      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP3, TMP1, TMP3);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTH(GetReg(Node), TMP1);
      break;
    }
    case 4: AMOAND_W(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOAND_D(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicFetchOr) {
  auto Op = IROp->C<IR::IROp_AtomicFetchOr>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      OR(TMP2, TMP2, TMP3);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP3, TMP1, -256);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTB(GetReg(Node), TMP1);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      OR(TMP2, TMP2, TMP3);

      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP3, TMP1, TMP3);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTH(GetReg(Node), TMP1);
      break;
    }
    case 4: AMOOR_W(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOOR_D(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicFetchXor) {
  auto Op = IROp->C<IR::IROp_AtomicFetchXor>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 8bit value
      UXTB(TMP2, TMP1);
      UXTB(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      XOR(TMP2, TMP2, TMP3);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP3, TMP1, -256);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTB(GetReg(Node), TMP1);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);
      // Extract 16bit value
      UXTH(TMP2, TMP1);
      UXTH(TMP3, GetReg(Op->Value.ID()));

      // Do the op
      XOR(TMP2, TMP2, TMP3);

      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP3, TMP1, TMP3);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTH(GetReg(Node), TMP1);
      break;
    }
    case 4: AMOXOR_W(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    case 8: AMOXOR_D(Ordering::AQRL, GetReg(Node), GetReg(Op->Value.ID()), MemSrc); break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

DEF_OP(AtomicFetchNeg) {
  auto Op = IROp->C<IR::IROp_AtomicFetchNeg>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    switch (IROp->Size) {
    case 1: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);

      // Do the op
      NEG(TMP2, TMP1);

      // Extract 8bit result
      UXTB(TMP2, TMP2);

      // Mask out 8bit bottom from source
      ANDI(TMP3, TMP1, -256);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTB(GetReg(Node), TMP1);
      break;
    }
    case 2: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);

      // Do the op
      NEG(TMP2, TMP1);

      // Extract 16bit result
      UXTH(TMP2, TMP2);

      // Mask out 16bit bottom from source
      // LUI loads the top 20 bits
      LUI(TMP3, 0xFFFF'0000 >> 12);
      AND(TMP3, TMP1, TMP3);

      // Insert result
      OR(TMP3, TMP3, TMP2);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTH(GetReg(Node), TMP1);
      break;
    }
    case 4: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_W(Ordering::AQRL, TMP1, MemSrc);

      // Do the op
      NEG(TMP2, TMP1);

      // Try to store
      SC_W(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);

      // TMP1 contains the original memory value
      UXTW(GetReg(Node), TMP1);
      break;
    }
    case 8: {
      // Prepare the emulation pain
      // 1) We must load a 32-bit value at the memory offset
      // 2) Extract the size of the element from the value
      // 3) Do the atomic operation
      // 4) Insert the value back in
      // 5) Atomically store and hope we still have ownership of the value
      // 6) Try again on failure
      Label TryAgain{};
      Bind(&TryAgain);
      LR_D(Ordering::AQRL, GetReg(Node), MemSrc);

      // Do the op
      NEG(TMP2, TMP1);

      // Try to store
      SC_D(Ordering::AQRL, TMP4, TMP3, MemSrc);
      BNEZ(TMP4, &TryAgain);
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
    }
  }
  else {
    ERROR_AND_DIE_FMT("Nope: {}", __func__);
  }
}

#undef DEF_OP
void RISCVJITCore::RegisterAtomicHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
  REGISTER_OP(CASPAIR,        CASPair);
  REGISTER_OP(CAS,            CAS);
  REGISTER_OP(ATOMICADD,      AtomicAdd);
  REGISTER_OP(ATOMICSUB,      AtomicSub);
  REGISTER_OP(ATOMICAND,      AtomicAnd);
  REGISTER_OP(ATOMICOR,       AtomicOr);
  REGISTER_OP(ATOMICXOR,      AtomicXor);
  REGISTER_OP(ATOMICSWAP,     AtomicSwap);
  REGISTER_OP(ATOMICFETCHADD, AtomicFetchAdd);
  REGISTER_OP(ATOMICFETCHSUB, AtomicFetchSub);
  REGISTER_OP(ATOMICFETCHAND, AtomicFetchAnd);
  REGISTER_OP(ATOMICFETCHOR,  AtomicFetchOr);
  REGISTER_OP(ATOMICFETCHXOR, AtomicFetchXor);
  REGISTER_OP(ATOMICFETCHNEG, AtomicFetchNeg);
#undef REGISTER_OP
}
}

