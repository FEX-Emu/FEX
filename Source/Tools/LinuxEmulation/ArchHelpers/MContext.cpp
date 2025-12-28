// SPDX-License-Identifier: MIT
#include "ArchHelpers/MContext.h"

namespace FEX::ArchHelpers::Context {
#ifdef ARCHITECTURE_arm64
std::string_view GetESRName(uint64_t ESR) {
  switch ((ESR & ESR1_EC) >> 26) {
  case 0b000'000: return "Unknown";
  case 0b000'001: return "Trapped WF*";
  case 0b000'011: return "Trapped MCR/MRC";
  case 0b000'100: return "Trapped MCRR/MRRC";
  case 0b000'101: return "Trapped MCR/MRC (coproc==0b1110)";
  case 0b000'110: return "Trapped LDC/STC";
  case 0b000'111: return "Trapped SME;SVE,ASIMD,FP";
  case 0b001'010: return "Trapped non-covered instruction";
  case 0b001'100: return "Trapped MRRC (coproc==0b1110)";
  case 0b001'101: return "Branch target exception";
  case 0b001'110: return "Illegal Execution State";
  case 0b010'001: return "AArch32 SVC";
  case 0b010'100: return "Trapped MSRR/MRRS/System instruction";
  case 0b010'101: return "AArch64 SVC";
  case 0b011'000: return "Trapped MSR/MRS/System instruction";
  case 0b011'001: return "Trapped SVE from ZEN";
  case 0b011'011: return "TSTART Exception";
  case 0b011'100: return "PAC Exception";
  case 0b011'101: return "Trapped SME from SMEN";
  case 0b100'000: return "Instruction abort";
  case 0b100'001: return "Instruction abort w/o change to exception level";
  case 0b100'010: return "PC Alignment fault";
  case 0b100'100: return "Data abort";
  case 0b100'101: return "Data abort w/o change to exception level";
  case 0b100'110: return "SP Alignment fault";
  case 0b100'111: return "Memory operation exception";
  case 0b101'000: return "AArch32 Trapped FP Exception";
  case 0b101'100: return "AArch64 Trapped FP Exception";
  case 0b101'101: return "GCS exception";
  case 0b101'111: return "SError exception";
  case 0b110'000: return "BP Exception";
  case 0b110'001: return "BP Exception w/o change to exception level";
  case 0b110'010: return "Software step Exception";
  case 0b110'011: return "Software step Exception w/o change to exception level";
  case 0b110'100: return "Watchpoint Exception";
  case 0b110'101: return "Watchpoit Exception w/o change to exception level";
  case 0b111'000: return "AArch32 BKPT";
  case 0b111'100: return "AArch64 BRK";
  case 0b111'101: return "Profiling Exception";
  default: return "Reserved";
  }
}
#endif
} // namespace FEX::ArchHelpers::Context
