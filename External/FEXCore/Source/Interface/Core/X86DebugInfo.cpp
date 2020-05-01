#ifndef NDEBUG
#include <FEXCore/Debug/X86Tables.h>
#include "LogManager.h"
#include <tuple>
#include <vector>

namespace FEXCore::X86Tables::X86InstDebugInfo {
void InstallDebugInfo() {

  using namespace FEXCore::X86Tables;
  auto NoFlags = Flags {0};

  for (auto &BaseOp : BaseOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : SecondBaseOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : RepModOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : RepNEModOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : OpSizeModOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : PrimaryInstGroupOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : SecondInstGroupOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : SecondModRMTableOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : X87Ops)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : DDDNowOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : H0F38TableOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : H0F3ATableOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : VEXTableOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : VEXTableGroupOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : XOPTableOps)
      BaseOp.DebugInfo = NoFlags;
  for (auto &BaseOp : XOPTableGroupOps)
      BaseOp.DebugInfo = NoFlags;

  const std::vector<std::tuple<uint8_t, uint8_t, Flags>> BaseOpTable = {
    {0x50, 8, {FLAGS_MEM_ACCESS}},
    {0x58, 8, {FLAGS_MEM_ACCESS}},

    {0x68, 1, {FLAGS_MEM_ACCESS}},
    {0x6A, 1, {FLAGS_MEM_ACCESS}},

    {0xAA, 4, {FLAGS_MEM_ACCESS}},

    {0xC8, 1, {FLAGS_MEM_ACCESS}},

    {0xCC, 2, {FLAGS_DEBUG}},

    {0xD7, 1, {FLAGS_MEM_ACCESS}},

    {0xF1, 1, {FLAGS_DEBUG}},
    {0xF4, 1, {FLAGS_DEBUG}},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, Flags>> TwoByteOpTable = {
    {0x0B, 1, {FLAGS_DEBUG}},
    {0x19, 7, {FLAGS_DEBUG}},
    {0x28, 2, {FLAGS_MEM_ALIGN_16}},

    {0x31, 1, {FLAGS_DEBUG}},

    {0xA2, 1, {FLAGS_DEBUG}},
    {0xA3, 1, {FLAGS_MEM_ACCESS}},
    {0xAB, 1, {FLAGS_MEM_ACCESS}},
    {0xB3, 1, {FLAGS_MEM_ACCESS}},
    {0xBB, 1, {FLAGS_MEM_ACCESS}},

    {0xFF, 1, {FLAGS_DEBUG}},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, Flags>> PrimaryGroupOpTable = {
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 6), 2, {FLAGS_DIVIDE}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 6), 2, {FLAGS_DIVIDE}},
#undef OPD
  };

  const std::vector<std::tuple<uint16_t, uint8_t, Flags>> SecondaryExtensionOpTable = {
#define PF_NONE 0
#define PF_F3 1
#define PF_66 2
#define PF_F2 3
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
    {OPD(TYPE_GROUP_15, PF_NONE, 2), 1, {FLAGS_DEBUG}},
    {OPD(TYPE_GROUP_15, PF_NONE, 3), 1, {FLAGS_DEBUG}},
#undef PF_F3
#undef PF_66
#undef PF_F2
#undef OPD
  };

  auto GenerateDebugTable = [](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto DebugInfo = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        memcpy(&FinalTable[OpNum+i].DebugInfo, &DebugInfo, sizeof(X86InstDebugInfo::Flags));
      }
    }
  };

  GenerateDebugTable(BaseOps, BaseOpTable);
  GenerateDebugTable(SecondBaseOps, TwoByteOpTable);
  GenerateDebugTable(PrimaryInstGroupOps, PrimaryGroupOpTable);

  GenerateDebugTable(SecondInstGroupOps, SecondaryExtensionOpTable);

  LogMan::Msg::D("Installing debug info");
}
}
#endif
