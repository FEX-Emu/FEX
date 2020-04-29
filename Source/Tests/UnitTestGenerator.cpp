#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"

#include "LogManager.h"
#include <cstdio>
#include <limits>
#include <vector>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/X86Tables.h>

constexpr std::array<std::pair<int16_t, int16_t>, 3> Disp8Ranges = {{
  {static_cast<int16_t>(-16), 16},
  {static_cast<int16_t>(-128), static_cast<int16_t>(-112)},
  {96, 112},
}};

constexpr std::array<std::pair<int64_t, int64_t>, 1> Disp32Ranges = {{
  {0, 32},
}};

int DumpThreshhold = (1 << 15);

void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
  const char *CharLevel{nullptr};

  switch (Level) {
  case LogMan::NONE:
    CharLevel = "NONE";
    break;
  case LogMan::ASSERT:
    CharLevel = "ASSERT";
    break;
  case LogMan::ERROR:
    CharLevel = "ERROR";
    break;
  case LogMan::DEBUG:
    CharLevel = "DEBUG";
    break;
  case LogMan::INFO:
    CharLevel = "Info";
    break;
  default:
    CharLevel = "???";
    break;
  }
  printf("[%s] %s\n", CharLevel, Message);
}

void AssertHandler(char const *Message) {
  printf("[ASSERT] %s\n", Message);
}

uint32_t GetModRMMapping(uint32_t Register) {
  switch (Register) {
  case FEXCore::X86State::REG_RCX: Register = 0b001; break;
  case FEXCore::X86State::REG_RDX: Register = 0b010; break;
  case FEXCore::X86State::REG_RBX: Register = 0b011; break;
  case FEXCore::X86State::REG_RSP: Register = 0b100; break;
  case FEXCore::X86State::REG_RBP: Register = 0b101; break;
  case FEXCore::X86State::REG_RSI: Register = 0b110; break;
  case FEXCore::X86State::REG_RDI: Register = 0b111; break;
  default: return Register; break; // Default mapping
  }
  return Register;
};

auto OpToIndex = [](uint8_t Op) constexpr -> uint8_t {
  switch (Op) {
  // Group 1
  case 0x80: return 0;
  case 0x81: return 1;
  case 0x82: return 2;
  case 0x83: return 3;
  // Group 2
  case 0xC0: return 0;
  case 0xC1: return 1;
  case 0xD0: return 2;
  case 0xD1: return 3;
  case 0xD2: return 4;
  case 0xD3: return 5;
  // Group 3
  case 0xF6: return 0;
  case 0xF7: return 1;
  // Group 4
  case 0xFE: return 0;
  // Group 5
  case 0xFF: return 0;
  // Group 11
  case 0xC6: return 0;
  case 0xC7: return 1;
  }
  return 0;
};

auto PrimaryIndexToOp = [](uint16_t Op) constexpr -> uint32_t {
#define OPD(group, prefix, Reg) ((((group) - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  switch (Op & ~0b111) {
  // Group 1
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 0): return 0x800000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 0): return 0x810000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x82), 0): return 0x820000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 0): return 0x830000 | (Op & 0b111);
  // Group 2
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 0): return 0xC00000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 0): return 0xC10000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 0): return 0xD00000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 0): return 0xD10000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 0): return 0xD20000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 0): return 0xD30000 | (Op & 0b111);
  // Group 3
  case OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 0): return 0xF60000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 0): return 0xF70000 | (Op & 0b111);
  // Group 4
  case OPD(FEXCore::X86Tables::TYPE_GROUP_4, OpToIndex(0xFE), 0): return 0xFE0000 | (Op & 0b111);
  // Group 5
  case OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 0): return 0xFF0000 | (Op & 0b111);
  // Group 11
  case OPD(FEXCore::X86Tables::TYPE_GROUP_11, OpToIndex(0xC6), 0): return 0xC60000 | (Op & 0b111);
  case OPD(FEXCore::X86Tables::TYPE_GROUP_11, OpToIndex(0xC7), 0): return 0xC70000 | (Op & 0b111);
  }
#undef OPD
  return 0;
};

auto SecondaryIndexToOp = [](uint16_t Op) constexpr -> uint32_t {
  constexpr std::array<uint32_t, FEXCore::X86Tables::TYPE_GROUP_P - FEXCore::X86Tables::TYPE_GROUP_6 + 1> GroupToOp = {
    0x000000, // 6
    0x010000, // 7
    0xBA0000, // 8
    0xC70000, // 9
    0xB90000, // 10
    0x710000, // 12 (11 is part of the primary op table
    0x720000, // 13
    0x730000, // 14
    0xAE0000, // 15
    0x180000, // 16
    0x780000, // 17
    0x0D0000, // P
  };
  constexpr std::array<uint32_t, 4> PrefixToOp = {
    0,
    0xF300,
    0x6600,
    0xF200,
  };
  return GroupToOp[Op >> 5] | PrefixToOp[(Op >> 3) & 0b11] | (Op & 0b111);
};

uint32_t GetModRMMappingXMM(uint32_t Register) {
  return Register - FEXCore::X86State::REG_XMM_0;
};

static std::vector<uint8_t> Code;
static std::string Filepath;
static std::string CurrentPrefix;
static int Step{};
static int TimeSinceLastDump{};

void GenerateMove(uint32_t Register, uint64_t Literal) {
  Register = GetModRMMapping(Register);
  int  Size = !!(Literal & (~0ULL << 32)) ? 8 : 4;
  uint8_t REX = 0x40 | (Size == 8 ? 0b1000 : 0);
  REX |= (Register & 0b1000) >> 3;
  Code.emplace_back(REX); // REX
  Code.emplace_back(0xB8 + (Register & 0b0111)); // MOV
  for (int i = 0; i < Size; ++i)
    Code.emplace_back(Literal >> (i * 8));
}

void Dump(std::string const &NameSuffix = "") {
  {
    // Dump a HLT at the end of the code
    Code.emplace_back(0xF4);
  }
  printf("Size: %zd Inst: %d\n", Code.size(), TimeSinceLastDump);
  std::string Filename = Filepath + "/" + CurrentPrefix + "_" + std::to_string(Step) + "_" + NameSuffix + ".raw";
  FILE *fp = fopen(Filename.c_str(), "wbe");
  fwrite(&Code[0], 1, Code.size(), fp);
  fclose(fp);
  Code.clear();
  Code.reserve(4096 * 128);
  ++Step;
  TimeSinceLastDump = 0;
}

void GeneratePrimaryTable() {
  using namespace FEXCore::X86Tables;
  int numInst {};
  Step = 0;
  TimeSinceLastDump = 0;
  CurrentPrefix = "Primary";

  bool AddressSizePrefix = false;
  auto DoNormalOps = [&](const char *NameSuffix, auto Inserter, std::optional<std::function<void()>> ModRMInserter, uint8_t REX = 0) {
    for (size_t OpIndex = 0; OpIndex < (sizeof(BaseOps) / sizeof(BaseOps[0])); ++OpIndex) {
      auto &Op = BaseOps[OpIndex];
      if (Op.Type == TYPE_INST) {
        if (Op.Flags & InstFlags::FLAGS_SETS_RIP ||
            Op.Flags & InstFlags::FLAGS_BLOCK_END) {
          continue;
        }

        if (OpIndex == 0x8D) { // LEA
          // Special case LEA
          // LEA with source as a register is INVALID and causes test harness generation to fail
          continue;
        }

        // Need to specialize these
        if (Op.Flags & InstFlags::FLAGS_MEM_OFFSET ||
            Op.Flags & InstFlags::FLAGS_DEBUG_MEM_ACCESS ||
            Op.Flags & InstFlags::FLAGS_DEBUG) {
          continue;
        }

        if (Op.Flags & InstFlags::FLAGS_MODRM &&
            !ModRMInserter.has_value()) {
          continue;
        }

        Inserter();
        Code.push_back(OpIndex);

        if (Op.Flags & InstFlags::FLAGS_MODRM) {
          ModRMInserter.value()();
        }

        if (Op.MoreBytes != 0) {
          uint32_t MoreBytes = Op.MoreBytes;

          if (REX & 0b1000) {
            if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_MUL_2) {
              MoreBytes <<= 1;
            }
          }
          if (AddressSizePrefix) {
            if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_DIV_2) {
              MoreBytes >>= 1;
            }
          }

          constexpr uint64_t Constant = 0xDEADBEEFBAD0DAD1ULL;
          for (uint32_t i = 0; i < MoreBytes; ++i) {
            Code.push_back(Constant >> (i * 8));
          }
        }
        numInst++;
#ifndef NDEBUG
        Op.NumUnitTestsGenerated++;
#endif

        TimeSinceLastDump++;
        if (TimeSinceLastDump >= DumpThreshhold) {
          Dump(NameSuffix);
        }
      }
    }
  };

  auto EmptyInserter = [](){};
  DoNormalOps("", EmptyInserter, {});
  Dump();

  for (uint8_t REX = 0x40; REX < 0x50; ++REX) {
    auto Inserter = [REX]() {
      Code.push_back(REX);
    };
    DoNormalOps("", Inserter, {}, REX);
  }
  Dump();

  for (uint8_t Prefix = 0x64; Prefix < 0x66; ++Prefix) {
    auto Inserter = [Prefix]() {
      Code.push_back(Prefix);
    };
    DoNormalOps("", Inserter, {});
  }
  Dump();

  AddressSizePrefix = true;
  for (uint8_t Prefix = 0x66; Prefix < 0x67; ++Prefix) {
    auto Inserter = [Prefix]() {
      Code.push_back(Prefix);
    };
    DoNormalOps("", Inserter, {});
  }
  AddressSizePrefix = false;
  for (uint8_t Prefix = 0x67; Prefix < 0x68; ++Prefix) {
    auto Inserter = [Prefix]() {
      Code.push_back(Prefix);
    };
    DoNormalOps("", Inserter, {});
  }

  Dump();

  for (uint8_t REX = 0x40; REX < 0x50; ++REX) {
    for (uint8_t Prefix = 0x64; Prefix < 0x66; ++Prefix) {
      auto Inserter = [REX, Prefix]() {
        Code.push_back(Prefix);
        Code.push_back(REX);
      };
      DoNormalOps("", Inserter, {}, REX);
    }
  }
  Dump();

  // 00 = register direct
  //    - rm = 100 = SIB - Supported
  //    - rm = 101 = disp32
  // 01 = Register direct + displacement8
  //    - rm = 100 = SIB + disp8 - Supported
  // 10 = register direct + displacement32
  //   - rm = 100 = SIB + disp32 - Supported
  {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
      for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
        uint8_t ModRM = (ModRM_mod << 6) |
          (ModRM_reg << 3) |
          ModRM_rm;
        auto Inserter = [ModRM]() {
          Code.push_back(ModRM);
        };
        DoNormalOps("ModRM", EmptyInserter, Inserter);
      }
    }
    Dump("ModRM");
  }

  // For register direct we need to load a memory region in to a register
  // Memory region is at [0xe000'0000, 0xf000'0000]
  // Drop ourselves right in the middle for testing
  {
    uint8_t ModRM_mod = 0b00;
    const std::vector <uint32_t> Registers = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RBX,
      FEXCore::X86State::REG_RCX,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDI,
      // FEXCore::X86State::REG_RBP, // mod=00 = RIP relative addressing
      // FEXCore::X86State::REG_RSP, // SIB
      FEXCore::X86State::REG_R8,
      FEXCore::X86State::REG_R9,
      FEXCore::X86State::REG_R10,
      FEXCore::X86State::REG_R11,
      // FEXCore::X86State::REG_R12, // mod=00 = RIP RELATIVE. Ignores bit in REX
      // FEXCore::X86State::REG_R13, // SIB. Ignores bit in REX
      FEXCore::X86State::REG_R14,
      FEXCore::X86State::REG_R15,
    };
    for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
      for (auto RM : Registers) {
        uint8_t ModRM_rm = GetModRMMapping(RM);

        uint8_t ModRM = (ModRM_mod << 6) |
          (ModRM_reg << 3) |
          (ModRM_rm & 0b111);
        auto PreInserter = [ModRM_rm, RM]() {
          GenerateMove(RM, 0xe000'0000 + 0x800'0000);

          if (ModRM_rm & 0b1000) {
            // We need REX for this
            uint8_t REX = 0x40;
            REX |= (ModRM_rm & 0b1000) >> 3;
            Code.emplace_back(REX); // REX
          }

        };

        auto Inserter = [ModRM]() {
          // Before we do anything, set this register to our memory region
          Code.push_back(ModRM);
        };
        DoNormalOps("ModRM", PreInserter, Inserter);
      }
    }
    Dump("ModRM");
  }

  // For register indirect we need to load a memory region in to a register
  // Memory region is at [0xe000'0000, 0xf000'0000]
  // Drop ourselves right in the middle for testing
  // Displacement is only 8bit here
  {
    uint8_t ModRM_mod = 0b01;
    const std::vector<uint32_t> Registers = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RBX,
      FEXCore::X86State::REG_RCX,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDI,
      FEXCore::X86State::REG_RBP,
      FEXCore::X86State::REG_R8,
      FEXCore::X86State::REG_R9,
      FEXCore::X86State::REG_R10,
      FEXCore::X86State::REG_R11,
      FEXCore::X86State::REG_R13,
      FEXCore::X86State::REG_R14,
      FEXCore::X86State::REG_R15,
    };
    for (auto DispRange : Disp8Ranges) {
      for(int16_t disp8 = DispRange.first; disp8 <= DispRange.second; disp8 += 16) {
        for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
          for (auto RM : Registers) {
            uint8_t ModRM_rm = GetModRMMapping(RM);

            uint8_t ModRM = (ModRM_mod << 6) |
              (ModRM_reg << 3) |
              (ModRM_rm & 0b111);
            auto PreInserter = [ModRM_rm, RM]() {
              GenerateMove(RM, 0xe000'0000 + 0x800'0000);

              if (ModRM_rm & 0b1000) {
                // We need REX for this
                uint8_t REX = 0x40;
                REX |= (ModRM_rm & 0b1000) >> 3;
                Code.emplace_back(REX); // REX
              }

            };

            auto Inserter = [ModRM, disp8]() {
              // Before we do anything, set this register to our memory region
              Code.push_back(ModRM);
              // Disp8 bit follows ModRM
              Code.push_back(disp8);
            };
            DoNormalOps("ModRM", PreInserter, Inserter);
          }
        }
      }
    }
    Dump("ModRM");
  }

  {
    const std::vector<uint32_t> RegistersIndex = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RBX,
      FEXCore::X86State::REG_RCX,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDI,
      FEXCore::X86State::REG_RBP,
      // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
      FEXCore::X86State::REG_R8,
      FEXCore::X86State::REG_R9,
      FEXCore::X86State::REG_R10,
      FEXCore::X86State::REG_R11,
      // FEXCore::X86State::REG_R12, // = 0
      FEXCore::X86State::REG_R13,
      FEXCore::X86State::REG_R14,
      FEXCore::X86State::REG_R15,
    };

    const std::vector<uint32_t> RegistersBase = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RBX,
      FEXCore::X86State::REG_RCX,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDI,
      // FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      FEXCore::X86State::REG_RSP,
      FEXCore::X86State::REG_R8,
      FEXCore::X86State::REG_R9,
      FEXCore::X86State::REG_R10,
      FEXCore::X86State::REG_R11,
      FEXCore::X86State::REG_R12,
      // FEXCore::X86State::REG_R13, // ModRM = 0b00 = base = 0
      FEXCore::X86State::REG_R14,
      FEXCore::X86State::REG_R15,
    };

    uint8_t ModRM_mod = 0b00;
    uint8_t ModRM_rm = 0b100;
    for (uint32_t RegIndex : RegistersIndex) {
      for (uint32_t RegBase : RegistersBase) {
        for (uint8_t scale = 0; scale < 1; ++scale) {
          for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
            if (RegIndex == RegBase) {
              // Skip these
              continue;
            }
            uint8_t IndexReg = GetModRMMapping(RegIndex);
            uint8_t BaseReg = GetModRMMapping(RegBase);
            uint8_t ModRM = (ModRM_mod << 6) |
              (ModRM_reg << 3) |
              ModRM_rm;

            uint8_t REX = 0x40;
            REX |= (IndexReg & 0b1000) >> 2;
            REX |= (BaseReg & 0b1000) >> 3;

            uint8_t index = IndexReg & 0b111;
            uint8_t base = BaseReg & 0b111;
            uint8_t SIB =
              (scale << 6) |
              (index << 3) |
              base;

            auto PreInserter = [REX, RegIndex, RegBase]() {
              GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                    GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
              Code.emplace_back(REX); // REX
            };

            auto Inserter = [ModRM, &SIB]() {
              Code.push_back(ModRM);
              Code.push_back(SIB); // SIB
            };
            DoNormalOps("ModRM_SIB", PreInserter, Inserter, REX);
          }
        }
      }
    }
    Dump("ModRM_SIB");
  }

  {
    const std::vector<uint32_t> RegistersIndex = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RBX,
      FEXCore::X86State::REG_RCX,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDI,
      FEXCore::X86State::REG_RBP,
      // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
      FEXCore::X86State::REG_R8,
      FEXCore::X86State::REG_R9,
      FEXCore::X86State::REG_R10,
      FEXCore::X86State::REG_R11,
      // FEXCore::X86State::REG_R12, // = 0
      FEXCore::X86State::REG_R13,
      FEXCore::X86State::REG_R14,
      FEXCore::X86State::REG_R15,
    };

    const std::vector<uint32_t> RegistersBase = {
      FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
    };

    uint8_t ModRM_mod = 0b01;
    uint8_t ModRM_rm = 0b100;
    for (uint32_t RegIndex : RegistersIndex) {
      for (uint32_t RegBase : RegistersBase) {
        for (uint8_t scale = 0; scale < 1; ++scale) {
          for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
            for (auto DispRange : Disp8Ranges) {
              for(int16_t disp8 = DispRange.first; disp8 <= DispRange.second; disp8 += 16) {
                if (RegIndex == RegBase) {
                  // Skip these
                  continue;
                }
                uint8_t IndexReg = GetModRMMapping(RegIndex);
                uint8_t BaseReg = GetModRMMapping(RegBase);
                uint8_t ModRM = (ModRM_mod << 6) |
                  (ModRM_reg << 3) |
                  ModRM_rm;

                uint8_t REX = 0x40;
                REX |= (IndexReg & 0b1000) >> 2;
                REX |= (BaseReg & 0b1000) >> 3;

                uint8_t index = IndexReg & 0b111;
                uint8_t base = BaseReg & 0b111;
                uint8_t SIB =
                  (scale << 6) |
                  (index << 3) |
                  base;

                auto PreInserter = [REX, RegIndex, RegBase]() {
                  GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                                      GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                  Code.emplace_back(REX); // REX
                };

                auto Inserter = [ModRM, &SIB, disp8]() {
                  Code.push_back(ModRM);
                  Code.push_back(SIB); // SIB
                  Code.push_back(disp8);
                };
                DoNormalOps("ModRM_SIB_disp8", PreInserter, Inserter, REX);
              }
            }
          }
        }
      }
    }
    Dump("ModRM_SIB_disp8");
  }

  {
    const std::vector<uint32_t> RegistersIndex = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RBX,
      FEXCore::X86State::REG_RCX,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDI,
      FEXCore::X86State::REG_RBP,
      // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
      FEXCore::X86State::REG_R8,
      FEXCore::X86State::REG_R9,
      FEXCore::X86State::REG_R10,
      FEXCore::X86State::REG_R11,
      // FEXCore::X86State::REG_R12, // = 0
      FEXCore::X86State::REG_R13,
      FEXCore::X86State::REG_R14,
      FEXCore::X86State::REG_R15,
    };

    const std::vector<uint32_t> RegistersBase = {
      FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
    };

    uint8_t ModRM_mod = 0b10;
    uint8_t ModRM_rm = 0b100;
    for (uint32_t RegIndex : RegistersIndex) {
      for (uint32_t RegBase : RegistersBase) {
        for (uint8_t scale = 0; scale < 1; ++scale) {
          for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
            for (auto DispRange : Disp32Ranges) {
              for(int64_t disp32 = DispRange.first; disp32 <= DispRange.second; disp32 += 16) {
                if (RegIndex == RegBase) {
                  // Skip these
                  continue;
                }
                uint8_t IndexReg = GetModRMMapping(RegIndex);
                uint8_t BaseReg = GetModRMMapping(RegBase);
                uint8_t ModRM = (ModRM_mod << 6) |
                  (ModRM_reg << 3) |
                  ModRM_rm;

                uint8_t REX = 0x40;
                REX |= (IndexReg & 0b1000) >> 2;
                REX |= (BaseReg & 0b1000) >> 3;

                uint8_t index = IndexReg & 0b111;
                uint8_t base = BaseReg & 0b111;
                uint8_t SIB =
                  (scale << 6) |
                  (index << 3) |
                  base;

                auto PreInserter = [REX, RegIndex, RegBase]() {
                  GenerateMove(RegBase, 0x2'0000'0000 + 0x0'8000'0000);
                                      GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                  Code.emplace_back(REX); // REX
                };

                auto Inserter = [ModRM, &SIB, disp32]() {
                  int32_t disp = disp32;
                  Code.push_back(ModRM);
                  Code.push_back(SIB); // SIB
                  // Disp32 bit follows SIB
                  for (int i = 0; i < 4; ++i)
                    Code.push_back(disp >> (i * 8));
                };
                DoNormalOps("ModRM_SIB_disp32", PreInserter, Inserter, REX);
              }
            }
          }
        }
      }
    }
    Dump("ModRM_SIB_disp32");
  }

  // For register indirect we need to load a memory region in to a register
  // Memory region is at [0xe000'0000, 0xf000'0000]
  // Additional 4GB region is at [0x2'0000'0000, 0x3'0000'1000)
  // Drop ourselves right in the middle for testing
  // Displacement is 32bit here
  {
    uint8_t ModRM_mod = 0b10;
    const std::vector<uint32_t> Registers = {
      FEXCore::X86State::REG_RAX,
      FEXCore::X86State::REG_RBX,
      FEXCore::X86State::REG_RCX,
      FEXCore::X86State::REG_RDX,
      FEXCore::X86State::REG_RSI,
      FEXCore::X86State::REG_RDI,
      FEXCore::X86State::REG_RBP,
      FEXCore::X86State::REG_R8,
      FEXCore::X86State::REG_R9,
      FEXCore::X86State::REG_R10,
      FEXCore::X86State::REG_R11,
      FEXCore::X86State::REG_R13,
      FEXCore::X86State::REG_R14,
      FEXCore::X86State::REG_R15,
    };

    for (auto DispRange : Disp32Ranges) {
      for(int64_t disp32 = DispRange.first; disp32 <= DispRange.second; disp32 += 16) {
        for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
          for (auto RM : Registers) {
            uint8_t ModRM_rm = GetModRMMapping(RM);

            uint8_t ModRM = (ModRM_mod << 6) |
              (ModRM_reg << 3) |
              (ModRM_rm & 0b111);
            auto PreInserter = [ModRM_rm, RM]() {
              GenerateMove(RM, 0x2'0000'0000 + 0x0'8000'0000);

              if (ModRM_rm & 0b1000) {
                // We need REX for this
                uint8_t REX = 0x40;
                REX |= (ModRM_rm & 0b1000) >> 3;
                Code.emplace_back(REX); // REX
              }

            };

            auto Inserter = [ModRM, disp32]() {
              // Before we do anything, set this register to our memory region
              Code.push_back(ModRM);
              // Disp32 bit follows ModRM
              for (int i = 0; i < 4; ++i)
                Code.push_back(disp32 >> (i * 8));
            };
            DoNormalOps("ModRM", PreInserter, Inserter);
          }
        }
      }
    }
    Dump("ModRM");
  }

  for (uint8_t REX = 0x40; REX < 0x50; ++REX) {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
      for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
        uint8_t ModRM = (ModRM_mod << 6) |
          (ModRM_reg << 3) |
          ModRM_rm;
        auto REXInserter = [REX]() {
          Code.push_back(REX);
        };

        auto Inserter = [ModRM]() {
          Code.push_back(ModRM);
        };
        DoNormalOps("ModRM", REXInserter, Inserter, REX);
      }
    }
  }
  Dump("ModRM");

  printf("NumInsts: %d\n", numInst);
}

void GeneratePrimaryGroupTable() {
  using namespace FEXCore::X86Tables;
  int numInst {};
  Step = 0;
  TimeSinceLastDump = 0;
  CurrentPrefix = "PrimaryGroup";

  auto DoNormalOps = [&](const char *NameSuffix, auto SkipCheck, auto Inserter, auto &Table, std::optional<std::function<void(uint8_t)>> ModRMInserter, uint8_t REX = 0) {
    for (size_t OpIndex = 0; OpIndex < (sizeof(Table) / sizeof(Table[0])); ++OpIndex) {
      auto &Op = Table[OpIndex];
      if (Op.Type == TYPE_INST) {
        if (Op.Flags & InstFlags::FLAGS_SETS_RIP ||
            Op.Flags & InstFlags::FLAGS_BLOCK_END) {
          continue;
        }

        if (SkipCheck(Op)) {
          continue;
        }

        // Need to specialize these
        if (Op.Flags & InstFlags::FLAGS_MEM_OFFSET ||
            Op.Flags & InstFlags::FLAGS_DEBUG_MEM_ACCESS ||
            Op.Flags & InstFlags::FLAGS_DEBUG) {
          continue;
        }

        if (Op.Flags & InstFlags::FLAGS_MODRM &&
            !ModRMInserter.has_value()) {
          continue;
        }

#ifndef NDEBUG
        if (Op.DebugInfo.DebugFlags & FEXCore::X86Tables::X86InstDebugInfo::FLAGS_DIVIDE) {
          continue;
        }
#endif

        Inserter();
        uint32_t HexOp = PrimaryIndexToOp(OpIndex);
        Code.push_back(HexOp >> 16);

        // Op selection is from the reg field of modrm
        if (Op.Flags & InstFlags::FLAGS_MODRM) {
          ModRMInserter.value()(HexOp & 0b111);
        }

        if (Op.MoreBytes != 0) {
          uint32_t MoreBytes = Op.MoreBytes;

          if (REX & 0b1000) {
            if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_MUL_2) {
              MoreBytes <<= 1;
            }
          }

          constexpr uint64_t Constant = 0xDEADBEEFBAD0DAD1ULL;
          for (uint32_t i = 0; i < MoreBytes; ++i) {
            Code.push_back(Constant >> (i * 8));
          }
        }
        numInst++;
#ifndef NDEBUG
        Op.NumUnitTestsGenerated++;
#endif

        TimeSinceLastDump++;
        if (TimeSinceLastDump >= DumpThreshhold) {
          Dump(NameSuffix);
        }
      }
    }
  };

  auto EmptyInserter = [](){};

  // 00 = register direct
  //    - rm = 100 = SIB - Supported
  //    - rm = 101 = disp32
  // 01 = Register direct + displacement8
  //    - rm = 100 = SIB + disp8 - Supported
  // 10 = register direct + displacement32
  //   - rm = 100 = SIB + disp32 - Supported
  {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
      uint8_t ModRM = (ModRM_mod << 6) |
        (0 << 3) |
        ModRM_rm;
      auto Inserter = [ModRM](uint8_t reg_field) {
        Code.push_back(ModRM | (reg_field << 3));
      };
      auto SkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
      {
        if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
          return true;
        }
        if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_MEM_ONLY) {
          return true;
        }
        return false;
      };

      DoNormalOps("ModRM", SkipCheck, EmptyInserter, PrimaryInstGroupOps, Inserter);
    }
    Dump("ModRM");
  }

  {
    auto SIBFunction = [&DoNormalOps](const char *Name, auto &Table) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        // FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
        FEXCore::X86State::REG_RSP,
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        FEXCore::X86State::REG_R12,
        // FEXCore::X86State::REG_R13, // ModRM = 0b00 = base = 0
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      uint8_t ModRM_mod = 0b00;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            if (RegIndex == RegBase) {
              // Skip these
              continue;
            }
            uint8_t IndexReg = GetModRMMapping(RegIndex);
            uint8_t BaseReg = GetModRMMapping(RegBase);
            uint8_t ModRM = (ModRM_mod << 6) |
              (0 << 3) |
              ModRM_rm;

            uint8_t REX = 0x40;
            REX |= (IndexReg & 0b1000) >> 2;
            REX |= (BaseReg & 0b1000) >> 3;

            uint8_t index = IndexReg & 0b111;
            uint8_t base = BaseReg & 0b111;
            uint8_t SIB =
              (scale << 6) |
              (index << 3) |
              base;

            auto PreInserter = [REX, RegIndex, RegBase]() {
              GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                                  GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
              Code.emplace_back(REX); // REX
            };

            auto Inserter = [ModRM, &SIB](uint8_t reg_field) {
              Code.push_back(ModRM | (reg_field << 3));
              Code.push_back(SIB); // SIB
            };
            auto SkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
            {
              if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
                return true;
              }
              if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                return true;
              }
              return false;
            };

            DoNormalOps(Name, SkipCheck, PreInserter, Table, Inserter, REX);
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB", PrimaryInstGroupOps);
  }
  {
    auto SIBFunction = [&DoNormalOps](const char *Name, auto &Table) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      };

      uint8_t ModRM_mod = 0b01;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (auto DispRange : Disp8Ranges) {
              for(int16_t disp8 = DispRange.first; disp8 <= DispRange.second; disp8 += 16) {
                if (RegIndex == RegBase) {
                  // Skip these
                  continue;
                }
                uint8_t IndexReg = GetModRMMapping(RegIndex);
                uint8_t BaseReg = GetModRMMapping(RegBase);
                uint8_t ModRM = (ModRM_mod << 6) |
                  (0 << 3) |
                  ModRM_rm;

                uint8_t REX = 0x40;
                REX |= (IndexReg & 0b1000) >> 2;
                REX |= (BaseReg & 0b1000) >> 3;

                uint8_t index = IndexReg & 0b111;
                uint8_t base = BaseReg & 0b111;
                uint8_t SIB =
                  (scale << 6) |
                  (index << 3) |
                  base;

                auto PreInserter = [REX, RegIndex, RegBase]() {
                  GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                                      GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                  Code.emplace_back(REX); // REX
                };

                auto Inserter = [ModRM, &SIB, &disp8](uint8_t reg_field) {
                  Code.push_back(ModRM | (reg_field << 3));
                  Code.push_back(SIB); // SIB
                  Code.push_back(disp8);
                };
                auto SkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
                {
                  if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
                    return true;
                  }
                  if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                    return true;
                  }
                  return false;
                };

                DoNormalOps(Name, SkipCheck, PreInserter, Table, Inserter, REX);
              }
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB8", PrimaryInstGroupOps);
  }

  {
    auto SIBFunction = [&DoNormalOps](const char *Name, auto &Table) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      };

      uint8_t ModRM_mod = 0b10;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (auto DispRange : Disp32Ranges) {
              for(int64_t disp32 = DispRange.first; disp32 <= DispRange.second; disp32 += 16) {
                if (RegIndex == RegBase) {
                  // Skip these
                  continue;
                }
                uint8_t IndexReg = GetModRMMapping(RegIndex);
                uint8_t BaseReg = GetModRMMapping(RegBase);
                uint8_t ModRM = (ModRM_mod << 6) |
                  (0 << 3) |
                  ModRM_rm;

                uint8_t REX = 0x40;
                REX |= (IndexReg & 0b1000) >> 2;
                REX |= (BaseReg & 0b1000) >> 3;

                uint8_t index = IndexReg & 0b111;
                uint8_t base = BaseReg & 0b111;
                uint8_t SIB =
                  (scale << 6) |
                  (index << 3) |
                  base;

              auto PreInserter = [REX, RegIndex, RegBase]() {
                GenerateMove(RegBase, 0x2'0000'0000 + 0x0'8000'0000);
                                    GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                Code.emplace_back(REX); // REX
              };

                auto Inserter = [ModRM, &SIB, disp32](uint8_t reg_field) {
                  Code.push_back(ModRM | (reg_field << 3));
                  Code.push_back(SIB); // SIB
                  // Disp32 bit follows SIB
                  for (int i = 0; i < 4; ++i)
                    Code.push_back(disp32 >> (i * 8));
                };
                auto SkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
                {
                  if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
                    return true;
                  }
                  if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                    return true;
                  }
                  return false;
                };
                DoNormalOps(Name, SkipCheck, PreInserter, Table, Inserter, REX);

              }
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB32", PrimaryInstGroupOps);
  }

  printf("Primary Group NumInsts: %d\n", numInst);
}

void GenerateSecondaryTable() {
  using namespace FEXCore::X86Tables;
  int numInst {};
  Step = 0;
  TimeSinceLastDump = 0;
  CurrentPrefix = "Secondary";

  auto DoNormalOps = [&](const char *NameSuffix, auto SkipCheck, auto Inserter, std::optional<std::function<void()>> ModRMInserter, uint8_t REX = 0) {
    for (size_t OpIndex = 0; OpIndex < (sizeof(SecondBaseOps) / sizeof(SecondBaseOps[0])); ++OpIndex) {
      auto &Op = SecondBaseOps[OpIndex];
      if (Op.Type == TYPE_INST) {
        if (Op.Flags & InstFlags::FLAGS_SETS_RIP ||
            Op.Flags & InstFlags::FLAGS_BLOCK_END) {
          continue;
        }

        if (SkipCheck(Op)) {
          continue;
        }

        // Need to specialize these
        if (Op.Flags & InstFlags::FLAGS_MEM_OFFSET ||
            Op.Flags & InstFlags::FLAGS_DEBUG_MEM_ACCESS ||
            Op.Flags & InstFlags::FLAGS_DEBUG) {
          continue;
        }

        if (Op.Flags & InstFlags::FLAGS_MODRM &&
            !ModRMInserter.has_value()) {
          continue;
        }

        Inserter();
        Code.push_back(0x0F); // Escape op
        Code.push_back(OpIndex);

        if (Op.Flags & InstFlags::FLAGS_MODRM) {
          ModRMInserter.value()();
        }

        if (Op.MoreBytes != 0) {
          uint32_t MoreBytes = Op.MoreBytes;

          if (REX & 0b1000) {
            if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_MUL_2) {
              MoreBytes <<= 1;
            }
          }

          constexpr uint64_t Constant = 0xDEADBEEFBAD0DAD1ULL;
          for (uint32_t i = 0; i < MoreBytes; ++i) {
            Code.push_back(Constant >> (i * 8));
          }
        }
        numInst++;
#ifndef NDEBUG
        Op.NumUnitTestsGenerated++;
#endif

        TimeSinceLastDump++;
        if (TimeSinceLastDump >= DumpThreshhold) {
          Dump(NameSuffix);
        }
      }
    }
  };

  auto EmptyInserter = [](){};
  auto EmptySkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op) { return false; };
  DoNormalOps("", EmptySkipCheck, EmptyInserter, {});
  Dump();

  // 00 = register direct
  //    - rm = 100 = SIB - Supported
  //    - rm = 101 = disp32
  // 01 = Register direct + displacement8
  //    - rm = 100 = SIB + disp8 - Supported
  // 10 = register direct + displacement32
  //   - rm = 100 = SIB + disp32 - Supported
  {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
      for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
        uint8_t ModRM = (ModRM_mod << 6) |
          (ModRM_reg << 3) |
          ModRM_rm;
        auto Inserter = [ModRM]() {
          Code.push_back(ModRM);
        };
        auto SkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
        {
          if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
            return true;
          }
          if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_MEM_ONLY) {
            return true;
          }
          return false;
        };

        DoNormalOps("ModRM", SkipCheck, EmptyInserter, Inserter);
      }
    }
    Dump("ModRM");
  }

  {
    auto SIBFunction = [&DoNormalOps](const char *Name) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        // FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
        FEXCore::X86State::REG_RSP,
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        FEXCore::X86State::REG_R12,
        // FEXCore::X86State::REG_R13, // ModRM = 0b00 = base = 0
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      uint8_t ModRM_mod = 0b00;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
              if (RegIndex == RegBase) {
                // Skip these
                continue;
              }
              uint8_t IndexReg = GetModRMMapping(RegIndex);
              uint8_t BaseReg = GetModRMMapping(RegBase);
              uint8_t ModRM = (ModRM_mod << 6) |
                (ModRM_reg << 3) |
                ModRM_rm;

              uint8_t REX = 0x40;
              REX |= (IndexReg & 0b1000) >> 2;
              REX |= (BaseReg & 0b1000) >> 3;

              uint8_t index = IndexReg & 0b111;
              uint8_t base = BaseReg & 0b111;
              uint8_t SIB =
                (scale << 6) |
                (index << 3) |
                base;

              auto PreInserter = [REX, RegIndex, RegBase]() {
                GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                                    GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                Code.emplace_back(REX); // REX
              };

              auto Inserter = [ModRM, &SIB]() {
                Code.push_back(ModRM);
                Code.push_back(SIB); // SIB
              };
              auto SkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
              {
                if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
                  return true;
                }
                if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                  return true;
                }
                return false;
              };

              DoNormalOps(Name, SkipCheck, PreInserter, Inserter, REX);
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB");
  }

  {
    auto SIBFunction = [&DoNormalOps](const char *Name) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      };

      uint8_t ModRM_mod = 0b01;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
              for (auto DispRange : Disp8Ranges) {
                for(int16_t disp8 = DispRange.first; disp8 <= DispRange.second; disp8 += 16) {
                  if (RegIndex == RegBase) {
                    // Skip these
                    continue;
                  }
                  uint8_t IndexReg = GetModRMMapping(RegIndex);
                  uint8_t BaseReg = GetModRMMapping(RegBase);
                  uint8_t ModRM = (ModRM_mod << 6) |
                    (ModRM_reg << 3) |
                    ModRM_rm;

                  uint8_t REX = 0x40;
                  REX |= (IndexReg & 0b1000) >> 2;
                  REX |= (BaseReg & 0b1000) >> 3;

                  uint8_t index = IndexReg & 0b111;
                  uint8_t base = BaseReg & 0b111;
                  uint8_t SIB =
                    (scale << 6) |
                    (index << 3) |
                    base;

                  auto PreInserter = [REX, RegIndex, RegBase]() {
                    GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                                        GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                    Code.emplace_back(REX); // REX
                  };

                  auto Inserter = [ModRM, &SIB, disp8]() {
                    Code.push_back(ModRM);
                    Code.push_back(SIB); // SIB
                    Code.push_back(disp8);
                  };
                  auto SkipRegOnly = [](FEXCore::X86Tables::X86InstInfo const &Op) {
                    if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM &&
                        Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                      // Skip if the modrm source is reg only
                      return true;
                    }

                    if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS) {
                      return true;
                    }
                    return false;
                  };
                  DoNormalOps(Name, SkipRegOnly, PreInserter, Inserter, REX);

                }
              }
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB8");
  }

  {
    auto SIBFunction = [&DoNormalOps](const char *Name) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      };

      uint8_t ModRM_mod = 0b10;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
              for (auto DispRange : Disp32Ranges) {
                for(int64_t disp32 = DispRange.first; disp32 <= DispRange.second; disp32 += 16) {
                  if (RegIndex == RegBase) {
                    // Skip these
                    continue;
                  }
                  uint8_t IndexReg = GetModRMMapping(RegIndex);
                  uint8_t BaseReg = GetModRMMapping(RegBase);
                  uint8_t ModRM = (ModRM_mod << 6) |
                    (ModRM_reg << 3) |
                    ModRM_rm;

                  uint8_t REX = 0x40;
                  REX |= (IndexReg & 0b1000) >> 2;
                  REX |= (BaseReg & 0b1000) >> 3;

                  uint8_t index = IndexReg & 0b111;
                  uint8_t base = BaseReg & 0b111;
                  uint8_t SIB =
                    (scale << 6) |
                    (index << 3) |
                    base;

                auto PreInserter = [REX, RegIndex, RegBase]() {
                  GenerateMove(RegBase, 0x2'0000'0000 + 0x0'8000'0000);
                                      GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                  Code.emplace_back(REX); // REX
                };

                  auto Inserter = [ModRM, &SIB, disp32]() {
                    Code.push_back(ModRM);
                    Code.push_back(SIB); // SIB
                    int32_t udisp32 = static_cast<int32_t>(disp32);
                    // Disp32 bit follows SIB
                    for (int i = 0; i < 4; ++i) {
                      Code.push_back(udisp32 & 0xFF);
                      udisp32 >>= 8;
                    }
                  };
                  auto SkipRegOnly = [](FEXCore::X86Tables::X86InstInfo const &Op) {
                    if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM &&
                        Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                      // Skip if the modrm source is reg only
                      return true;
                    }

                    if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS) {
                      return true;
                    }
                    return false;
                  };
                  DoNormalOps(Name, SkipRegOnly, PreInserter, Inserter, REX);

                }
              }
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB32");
  }

  printf("Secondary NumInsts: %d\n", numInst);
}

void GenerateSecondaryGroupTable() {
  using namespace FEXCore::X86Tables;
  int numInst {};
  Step = 0;
  TimeSinceLastDump = 0;
  CurrentPrefix = "SecondaryGroup";

  auto DoNormalOps = [&](const char *NameSuffix, auto SkipCheck, auto Inserter, std::optional<std::function<void(uint8_t)>> ModRMInserter, uint8_t REX = 0) {
    for (size_t OpIndex = 0; OpIndex < (sizeof(SecondInstGroupOps) / sizeof(SecondInstGroupOps[0])); ++OpIndex) {
      auto &Op = SecondInstGroupOps[OpIndex];
      if (Op.Type == TYPE_INST) {
        if (Op.Flags & InstFlags::FLAGS_SETS_RIP ||
            Op.Flags & InstFlags::FLAGS_BLOCK_END) {
          continue;
        }

#ifndef NDEBUG
        if (Op.DebugInfo.DebugFlags & FEXCore::X86Tables::X86InstDebugInfo::FLAGS_DEBUG) {
          continue;
        }
#endif

        if (SkipCheck(Op)) {
          continue;
        }

        // Need to specialize these
        if (Op.Flags & InstFlags::FLAGS_MEM_OFFSET ||
            Op.Flags & InstFlags::FLAGS_DEBUG_MEM_ACCESS ||
            Op.Flags & InstFlags::FLAGS_DEBUG) {
          continue;
        }

        if (Op.Flags & InstFlags::FLAGS_MODRM &&
            !ModRMInserter.has_value()) {
          continue;
        }

        Inserter();
        uint32_t HexOp = SecondaryIndexToOp(OpIndex);

        uint32_t SecondaryEscapeOp = (HexOp >> 8) & 0xFF;
        if (SecondaryEscapeOp != 0)
          Code.push_back(SecondaryEscapeOp); // Secondary escape op
        Code.push_back(0x0F); // Escape op
        Code.push_back((HexOp >> 16) & 0xFF);

        // Op selection is from the reg field of modrm
        if (Op.Flags & InstFlags::FLAGS_MODRM) {
          ModRMInserter.value()(HexOp & 0b111);
        }

        if (Op.MoreBytes != 0) {
          uint32_t MoreBytes = Op.MoreBytes;

          if (REX & 0b1000) {
            if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_MUL_2) {
              MoreBytes <<= 1;
            }
          }

          constexpr uint64_t Constant = 0xDEADBEEFBAD0DAD1ULL;
          for (uint32_t i = 0; i < MoreBytes; ++i) {
            Code.push_back(Constant >> (i * 8));
          }
        }
        numInst++;
#ifndef NDEBUG
        Op.NumUnitTestsGenerated++;
#endif

        TimeSinceLastDump++;
        if (TimeSinceLastDump >= DumpThreshhold) {
          Dump(NameSuffix);
        }
      }
    }
  };

  auto EmptyInserter = [](){};

  // 00 = register direct
  //    - rm = 100 = SIB - Supported
  //    - rm = 101 = disp32
  // 01 = Register direct + displacement8
  //    - rm = 100 = SIB + disp8 - Supported
  // 10 = register direct + displacement32
  //   - rm = 100 = SIB + disp32 - Supported
  {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
      uint8_t ModRM = (ModRM_mod << 6) |
        (0 << 3) |
        ModRM_rm;
      auto Inserter = [ModRM](uint8_t reg_field) {
        Code.push_back(ModRM | (reg_field << 3));
      };
      auto SkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
      {
        if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
          return true;
        }
        if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_MEM_ONLY) {
          return true;
        }
        return false;
      };

      DoNormalOps("ModRM", SkipCheck, EmptyInserter, Inserter);
    }
    Dump("ModRM");
  }

  {
    auto SIBFunction = [&DoNormalOps](const char *Name) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        // FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
        FEXCore::X86State::REG_RSP,
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        FEXCore::X86State::REG_R12,
        // FEXCore::X86State::REG_R13, // ModRM = 0b00 = base = 0
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      uint8_t ModRM_mod = 0b00;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            if (RegIndex == RegBase) {
              // Skip these
              continue;
            }
            uint8_t IndexReg = GetModRMMapping(RegIndex);
            uint8_t BaseReg = GetModRMMapping(RegBase);
            uint8_t ModRM = (ModRM_mod << 6) |
              (0 << 3) |
              ModRM_rm;

            uint8_t REX = 0x40;
            REX |= (IndexReg & 0b1000) >> 2;
            REX |= (BaseReg & 0b1000) >> 3;

            uint8_t index = IndexReg & 0b111;
            uint8_t base = BaseReg & 0b111;
            uint8_t SIB =
              (scale << 6) |
              (index << 3) |
              base;

            auto PreInserter = [REX, RegIndex, RegBase]() {
              GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
              GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
              Code.emplace_back(REX); // REX
            };

            auto Inserter = [ModRM, &SIB](uint8_t reg_field) {
              Code.push_back(ModRM | (reg_field << 3));
              Code.push_back(SIB); // SIB
            };
            auto SkipCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
            {
              if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
                return true;
              }
              if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                return true;
              }
              return false;
            };

            DoNormalOps(Name, SkipCheck, PreInserter, Inserter, REX);
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB");
  }

  {
    auto SIBFunction = [&DoNormalOps](const char *Name) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      };

      uint8_t ModRM_mod = 0b01;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (auto DispRange : Disp8Ranges) {
              for(int16_t disp8 = DispRange.first; disp8 <= DispRange.second; disp8 += 16) {
                if (RegIndex == RegBase) {
                  // Skip these
                  continue;
                }
                uint8_t IndexReg = GetModRMMapping(RegIndex);
                uint8_t BaseReg = GetModRMMapping(RegBase);
                uint8_t ModRM = (ModRM_mod << 6) |
                  (0 << 3) |
                  ModRM_rm;

                uint8_t REX = 0x40;
                REX |= (IndexReg & 0b1000) >> 2;
                REX |= (BaseReg & 0b1000) >> 3;

                uint8_t index = IndexReg & 0b111;
                uint8_t base = BaseReg & 0b111;
                uint8_t SIB =
                  (scale << 6) |
                  (index << 3) |
                  base;

                auto PreInserter = [REX, RegIndex, RegBase]() {
                  GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                  GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                  Code.emplace_back(REX); // REX
                };

                auto Inserter = [ModRM, &SIB, disp8](uint8_t reg_field) {
                  Code.push_back(ModRM | (reg_field << 3));
                  Code.push_back(SIB); // SIB
                  Code.push_back(disp8);
                };
                auto SkipRegOnly = [](FEXCore::X86Tables::X86InstInfo const &Op) {
                  if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM &&
                      Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                    // Skip if the modrm source is reg only
                    return true;
                  }

                  if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS) {
                    return true;
                  }
                  return false;
                };
                DoNormalOps(Name, SkipRegOnly, PreInserter, Inserter, REX);

              }
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB8");
  }

  {
    auto SIBFunction = [&DoNormalOps](const char *Name) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      };

      uint8_t ModRM_mod = 0b10;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (auto DispRange : Disp32Ranges) {
              for(int64_t disp32 = DispRange.first; disp32 <= DispRange.second; disp32 += 16) {
                if (RegIndex == RegBase) {
                  // Skip these
                  continue;
                }
                uint8_t IndexReg = GetModRMMapping(RegIndex);
                uint8_t BaseReg = GetModRMMapping(RegBase);
                uint8_t ModRM = (ModRM_mod << 6) |
                  (0 << 3) |
                  ModRM_rm;

                uint8_t REX = 0x40;
                REX |= (IndexReg & 0b1000) >> 2;
                REX |= (BaseReg & 0b1000) >> 3;

                uint8_t index = IndexReg & 0b111;
                uint8_t base = BaseReg & 0b111;
                uint8_t SIB =
                  (scale << 6) |
                  (index << 3) |
                  base;

              auto PreInserter = [REX, RegIndex, RegBase]() {
                GenerateMove(RegBase, 0x2'0000'0000 + 0x0'8000'0000);
                GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                Code.emplace_back(REX); // REX
              };

                auto Inserter = [ModRM, &SIB, disp32](uint8_t reg_field) {
                  Code.push_back(ModRM | (reg_field << 3));
                  Code.push_back(SIB); // SIB
                  // Disp32 bit follows SIB
                  for (int i = 0; i < 4; ++i)
                    Code.push_back(disp32 >> (i * 8));
                };
                auto SkipRegOnly = [](FEXCore::X86Tables::X86InstInfo const &Op) {
                  if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM &&
                      Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY) {
                    // Skip if the modrm source is reg only
                    return true;
                  }

                  if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS) {
                    return true;
                  }
                  return false;
                };
                DoNormalOps(Name, SkipRegOnly, PreInserter, Inserter, REX);

              }
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction("ModRM_SIB32");
  }

  printf("Secondary Group NumInsts: %d\n", numInst);
}

void GenerateSSEInstructions() {
  using namespace FEXCore::X86Tables;
  int numInst {};
  Step = 0;
  TimeSinceLastDump = 0;
  CurrentPrefix = "SSE";

  bool AddressSizePrefix = false;

  auto DoNormalOps = [&](const char *NameSuffix, auto SkipCheck, auto Inserter, auto &Table, std::optional<std::function<void()>> ModRMInserter, uint8_t REX = 0) {
    for (size_t OpIndex = 0; OpIndex < (sizeof(Table) / sizeof(Table[0])); ++OpIndex) {
      auto &Op = Table[OpIndex];
      if (Op.Type == TYPE_INST) {
        if (Op.Flags & InstFlags::FLAGS_SETS_RIP ||
            Op.Flags & InstFlags::FLAGS_BLOCK_END) {
          continue;
        }
        if (!(Op.Flags & InstFlags::FLAGS_XMM_FLAGS)) {
          continue;
        }

        if (SkipCheck(Op)) {
          continue;
        }

        // Need to specialize these
        if (Op.Flags & InstFlags::FLAGS_MEM_OFFSET ||
            Op.Flags & InstFlags::FLAGS_DEBUG_MEM_ACCESS ||
            Op.Flags & InstFlags::FLAGS_DEBUG) {
          continue;
        }

        if (Op.Flags & InstFlags::FLAGS_MODRM &&
            !ModRMInserter.has_value()) {
          continue;
        }

        Inserter();
        Code.push_back(0x0F); // Need to escape to get in to this table
        Code.push_back(OpIndex);

        if (Op.Flags & InstFlags::FLAGS_MODRM) {
          ModRMInserter.value()();
        }

        if (Op.MoreBytes != 0) {
          uint32_t MoreBytes = Op.MoreBytes;

          if (REX & 0b1000) {
            if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_MUL_2) {
              MoreBytes <<= 1;
            }
          }
          if (AddressSizePrefix) {
            if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_DIV_2) {
              MoreBytes >>= 1;
            }
          }

          constexpr uint64_t Constant = 0xDEADBEEFBAD0DAD1ULL;
          for (uint32_t i = 0; i < MoreBytes; ++i) {
            Code.push_back(Constant >> (i * 8));
          }
        }
        numInst++;
#ifndef NDEBUG
        Op.NumUnitTestsGenerated++;
#endif

        TimeSinceLastDump++;
        if (TimeSinceLastDump >= DumpThreshhold) {
          Dump(NameSuffix);
        }
      }
    }
  };

  auto EmptyInserter = [](){};
  auto SkipMemOnlyCheck = [](FEXCore::X86Tables::X86InstInfo const &Op)
  {
    if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM)) {
      return true;
    }
    if (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_MEM_ONLY) {
      return true;
    }
    return false;
  };


  // 00 = register direct
  //    - rm = 100 = SIB - Supported
  //    - rm = 101 = disp32
  // 01 = Register direct + displacement8
  //    - rm = 100 = SIB + disp8 - Supported
  // 10 = register direct + displacement32
  //   - rm = 100 = SIB + disp32 - Supported
  {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
      for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
        uint8_t ModRM = (ModRM_mod << 6) |
          (ModRM_reg << 3) |
          ModRM_rm;
        auto Inserter = [ModRM]() {
          Code.push_back(ModRM);
        };

        DoNormalOps("ModRM", SkipMemOnlyCheck, EmptyInserter, SecondBaseOps, Inserter);
      }
    }
    Dump("ModRM");
  }
  {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
      for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
        uint8_t ModRM = (ModRM_mod << 6) |
          (ModRM_reg << 3) |
          ModRM_rm;
        auto Inserter = [ModRM]() {
          Code.push_back(ModRM);
        };
        auto REPInserter = []() {
          Code.push_back(0xF3);
        };
        DoNormalOps("ModRM_REP", SkipMemOnlyCheck, REPInserter, RepModOps, Inserter);
      }
    }
    Dump("ModRM_REP");
  }
  {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
      for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
        uint8_t ModRM = (ModRM_mod << 6) |
          (ModRM_reg << 3) |
          ModRM_rm;
        auto Inserter = [ModRM]() {
          Code.push_back(ModRM);
        };
        auto REPInserter = []() {
          Code.push_back(0xF2);
        };
        DoNormalOps("ModRM_REPNE", SkipMemOnlyCheck, REPInserter, RepNEModOps, Inserter);
      }
    }
    Dump("ModRM_REPNE");
  }
  {
    uint8_t ModRM_mod = 0b11;
    for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
      for (uint8_t ModRM_rm = 0; ModRM_rm < 8; ++ModRM_rm) {
        uint8_t ModRM = (ModRM_mod << 6) |
          (ModRM_reg << 3) |
          ModRM_rm;
        auto Inserter = [ModRM]() {
          Code.push_back(ModRM);
        };
        auto OpSizeInserter = []() {
          Code.push_back(0x66);
        };
        DoNormalOps("ModRM_OpSize", SkipMemOnlyCheck, OpSizeInserter, OpSizeModOps, Inserter);
      }
    }
    Dump("ModRM_OpSize");
  }

  {
    auto SIBFunction = [&DoNormalOps](uint8_t Prefix, const char *Name, auto &Table) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        // FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
        FEXCore::X86State::REG_RSP,
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        FEXCore::X86State::REG_R12,
        // FEXCore::X86State::REG_R13, // ModRM = 0b00 = base = 0
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      uint8_t ModRM_mod = 0b00;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
              if (RegIndex == RegBase) {
                // Skip these
                continue;
              }
              uint8_t IndexReg = GetModRMMapping(RegIndex);
              uint8_t BaseReg = GetModRMMapping(RegBase);
              uint8_t ModRM = (ModRM_mod << 6) |
                (ModRM_reg << 3) |
                ModRM_rm;

              uint8_t REX = 0x40;
              REX |= (IndexReg & 0b1000) >> 2;
              REX |= (BaseReg & 0b1000) >> 3;

              uint8_t index = IndexReg & 0b111;
              uint8_t base = BaseReg & 0b111;
              uint8_t SIB =
                (scale << 6) |
                (index << 3) |
                base;

              auto PreInserter = [REX, RegIndex, RegBase, Prefix]() {
                GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                Code.emplace_back(REX); // REX
                if (Prefix != 0) {
                  Code.emplace_back(Prefix);
                }
              };

              auto Inserter = [ModRM, &SIB]() {
                Code.push_back(ModRM);
                Code.push_back(SIB); // SIB
              };
              auto SkipRegOnly = [](FEXCore::X86Tables::X86InstInfo const &Op) {
                // Skip if the modrm source is reg only
                return Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM &&
                    Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY;
              };

              DoNormalOps(Name, SkipRegOnly, PreInserter, Table, Inserter, REX);
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction(0, "ModRM_SIB", SecondBaseOps);
    SIBFunction(0xF3, "ModRM_REP_SIB", RepModOps);
    SIBFunction(0xF2, "ModRM_REPNE_SIB", RepNEModOps);
    SIBFunction(0x66, "ModRM_OpSize_SIB", OpSizeModOps);
  }
  {
    auto SIBFunction = [&DoNormalOps](uint8_t Prefix, const char *Name, auto &Table) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      };

      uint8_t ModRM_mod = 0b01;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
              for (auto DispRange : Disp8Ranges) {
                for(int16_t disp8 = DispRange.first; disp8 <= DispRange.second; disp8 += 16) {
                  if (RegIndex == RegBase) {
                    // Skip these
                    continue;
                  }
                  uint8_t IndexReg = GetModRMMapping(RegIndex);
                  uint8_t BaseReg = GetModRMMapping(RegBase);
                  uint8_t ModRM = (ModRM_mod << 6) |
                    (ModRM_reg << 3) |
                    ModRM_rm;

                  uint8_t REX = 0x40;
                  REX |= (IndexReg & 0b1000) >> 2;
                  REX |= (BaseReg & 0b1000) >> 3;

                  uint8_t index = IndexReg & 0b111;
                  uint8_t base = BaseReg & 0b111;
                  uint8_t SIB =
                    (scale << 6) |
                    (index << 3) |
                    base;

                  auto PreInserter = [REX, RegIndex, RegBase, Prefix]() {
                    GenerateMove(RegBase, 0xe000'0000 + 0x800'0000);
                    GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                    Code.emplace_back(REX); // REX
                    if (Prefix != 0) {
                      Code.emplace_back(Prefix);
                    }
                  };

                  auto Inserter = [ModRM, &SIB, disp8]() {
                    Code.push_back(ModRM);
                    Code.push_back(SIB); // SIB
                    Code.push_back(disp8);
                  };
                  auto SkipRegOnly = [](FEXCore::X86Tables::X86InstInfo const &Op) {
                    // Skip if the modrm source is reg only
                    return Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM &&
                      Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY;
                  };

                  DoNormalOps(Name, SkipRegOnly, PreInserter, Table, Inserter, REX);
                }
              }
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction(0, "ModRM_SIB8", SecondBaseOps);
    SIBFunction(0xF3, "ModRM_REP_SIB8", RepModOps);
    SIBFunction(0xF2, "ModRM_REPNE_SIB8", RepNEModOps);
    SIBFunction(0x66, "ModRM_OpSize_SIB8", OpSizeModOps);
  }

  {
    auto SIBFunction = [&DoNormalOps](uint8_t Prefix, const char *Name, auto &Table) {
      const std::vector<uint32_t> RegistersIndex = {
        FEXCore::X86State::REG_RAX,
        FEXCore::X86State::REG_RBX,
        FEXCore::X86State::REG_RCX,
        FEXCore::X86State::REG_RDX,
        FEXCore::X86State::REG_RSI,
        FEXCore::X86State::REG_RDI,
        FEXCore::X86State::REG_RBP,
        // FEXCore::X86State::REG_RSP, // RSP = Scale*Index = 0
        FEXCore::X86State::REG_R8,
        FEXCore::X86State::REG_R9,
        FEXCore::X86State::REG_R10,
        FEXCore::X86State::REG_R11,
        // FEXCore::X86State::REG_R12, // = 0
        FEXCore::X86State::REG_R13,
        FEXCore::X86State::REG_R14,
        FEXCore::X86State::REG_R15,
      };

      const std::vector<uint32_t> RegistersBase = {
        FEXCore::X86State::REG_RBP, // ModRM = 0b00 = base = 0
      };

      uint8_t ModRM_mod = 0b10;
      uint8_t ModRM_rm = 0b100;
      for (uint32_t RegIndex : RegistersIndex) {
        for (uint32_t RegBase : RegistersBase) {
          for (uint8_t scale = 0; scale < 1; ++scale) {
            for (uint8_t ModRM_reg = 0; ModRM_reg < 8; ++ModRM_reg) {
              for (auto DispRange : Disp32Ranges) {
                for(int64_t disp32 = DispRange.first; disp32 <= DispRange.second; disp32 += 16) {
                  if (RegIndex == RegBase) {
                    // Skip these
                    continue;
                  }
                  uint8_t IndexReg = GetModRMMapping(RegIndex);
                  uint8_t BaseReg = GetModRMMapping(RegBase);
                  uint8_t ModRM = (ModRM_mod << 6) |
                    (ModRM_reg << 3) |
                    ModRM_rm;

                  uint8_t REX = 0x40;
                  REX |= (IndexReg & 0b1000) >> 2;
                  REX |= (BaseReg & 0b1000) >> 3;

                  uint8_t index = IndexReg & 0b111;
                  uint8_t base = BaseReg & 0b111;
                  uint8_t SIB =
                    (scale << 6) |
                    (index << 3) |
                    base;

                  auto PreInserter = [REX, RegIndex, RegBase, Prefix]() {
                    GenerateMove(RegBase, 0x2'0000'0000 + 0x0'8000'0000);
                    GenerateMove(RegIndex, 16); // Just ensure 128bit alignment
                    Code.emplace_back(REX); // REX
                    if (Prefix != 0) {
                      Code.emplace_back(Prefix);
                    }
                  };

                  auto Inserter = [ModRM, &SIB, disp32]() {
                    Code.push_back(ModRM);
                    Code.push_back(SIB); // SIB
                    // Disp32 bit follows SIB
                    for (int i = 0; i < 4; ++i)
                      Code.push_back(disp32 >> (i * 8));
                  };
                  auto SkipRegOnly = [](FEXCore::X86Tables::X86InstInfo const &Op) {
                    if ((Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM) &&
                      (Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_REG_ONLY)) {
                      // Skip if the modrm source is reg only
                      return true;
                    }
                    if (!(Op.Flags & FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS)) {
                      return true;
                    }

                    return false;
                  };

                  DoNormalOps(Name, SkipRegOnly, PreInserter, Table, Inserter, REX);
                }
              }
            }
          }
        }
      }
      Dump(Name);
    };

    SIBFunction(0, "ModRM_SIB32", SecondBaseOps);
    SIBFunction(0xF3, "ModRM_REP_SIB32", RepModOps);
    SIBFunction(0xF2, "ModRM_REPNE_SIB32", RepNEModOps);
    SIBFunction(0x66, "ModRM_OpSize_SIB32", OpSizeModOps);
  }

  printf("SSE NumInsts: %d\n", numInst);
}

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  FEX::Config::Init();
  FEX::EnvLoader::Load(envp);
  FEX::ArgLoader::Load(argc, argv);

  auto Args = FEX::ArgLoader::Get();

  LogMan::Throw::A(!Args.empty(), "Not enough arguments");

  FEXCore::X86Tables::InitializeInfoTables(FEXCore::Context::MODE_64BIT);

  Code.reserve(4096*128);
  Filepath = Args[0];

  GeneratePrimaryTable();
  GeneratePrimaryGroupTable();
  GenerateSecondaryTable();
  GenerateSecondaryGroupTable();
  GenerateSSEInstructions();

#ifndef NDEBUG
  if (Args.size() > 1) {
    using namespace FEXCore::X86Tables;
    auto DumpTable = [](std::string const &Filepath, std::string const &TableName, auto &Table) {
      std::string Filename = Filepath + "/" + TableName + ".csv";
      FILE *fp = fopen(Filename.c_str(), "wbe");

      fprintf(fp, "HEX, Name, Num Times compiled\n");
      for (size_t OpIndex = 0; OpIndex < (sizeof(Table) / sizeof(Table[0])); ++OpIndex) {
        auto &Op = Table[OpIndex];
        if (Op.Type == TYPE_INST) {
          fprintf(fp, "0x%zx, %s, %d\n", OpIndex, Op.Name, Op.NumUnitTestsGenerated);
        }
      }
      fclose(fp);
    };

    DumpTable(Args[1], "Primary", BaseOps);
    DumpTable(Args[1], "Secondary", SecondBaseOps);
    DumpTable(Args[1], "Secondary_REP", RepModOps);
    DumpTable(Args[1], "Secondary_REPNE", RepNEModOps);
    DumpTable(Args[1], "Secondary_OpSize", OpSizeModOps);
    DumpTable(Args[1], "Primary_Groups", PrimaryInstGroupOps);
    DumpTable(Args[1], "Secondary_Groups", SecondInstGroupOps);
    DumpTable(Args[1], "SecondaryModRM", SecondModRMTableOps);
  }
#endif

  return 0;
}

