#include "LogManager.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/X86Tables.h>
#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace FEXCore::X86Tables {

X86InstInfo BaseOps[MAX_PRIMARY_TABLE_SIZE];

X86InstInfo SecondBaseOps[MAX_SECOND_TABLE_SIZE];
X86InstInfo RepModOps[MAX_REP_MOD_TABLE_SIZE];
X86InstInfo RepNEModOps[MAX_REPNE_MOD_TABLE_SIZE];
X86InstInfo OpSizeModOps[MAX_OPSIZE_MOD_TABLE_SIZE];

X86InstInfo PrimaryInstGroupOps[MAX_INST_GROUP_TABLE_SIZE];
X86InstInfo SecondInstGroupOps[MAX_INST_SECOND_GROUP_TABLE_SIZE];
X86InstInfo SecondModRMTableOps[MAX_SECOND_MODRM_TABLE_SIZE];
X86InstInfo X87Ops[MAX_X87_TABLE_SIZE];
X86InstInfo DDDNowOps[MAX_3DNOW_TABLE_SIZE];
X86InstInfo H0F38TableOps[MAX_0F_38_TABLE_SIZE];
X86InstInfo H0F3ATableOps[MAX_0F_3A_TABLE_SIZE];
X86InstInfo VEXTableOps[MAX_VEX_TABLE_SIZE];
X86InstInfo VEXTableGroupOps[MAX_VEX_GROUP_TABLE_SIZE];
X86InstInfo XOPTableOps[MAX_XOP_TABLE_SIZE];
X86InstInfo XOPTableGroupOps[MAX_XOP_GROUP_TABLE_SIZE];
X86InstInfo EVEXTableOps[MAX_EVEX_TABLE_SIZE];

void InitializeBaseTables(Context::OperatingMode Mode);
void InitializeSecondaryTables();
void InitializePrimaryGroupTables();
void InitializeSecondaryGroupTables();
void InitializeSecondaryModRMTables();
void InitializeX87Tables();
void InitializeDDDTables();
void InitializeH0F38Tables();
void InitializeH0F3ATables();
void InitializeVEXTables();
void InitializeXOPTables();
void InitializeEVEXTables();

#ifndef NDEBUG
uint64_t Total{};
uint64_t NumInsts{};
#endif

void InitializeInfoTables(Context::OperatingMode Mode) {
  using namespace FEXCore::X86Tables::InstFlags;
  auto UnknownOp = X86InstInfo{"UND", TYPE_UNKNOWN, FLAGS_NONE, 0, nullptr};

  for (auto &BaseOp : BaseOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : SecondBaseOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : RepModOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : RepNEModOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : OpSizeModOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : PrimaryInstGroupOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : SecondInstGroupOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : SecondModRMTableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : X87Ops)
      BaseOp = UnknownOp;
  for (auto &BaseOp : DDDNowOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : H0F38TableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : H0F3ATableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : VEXTableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : VEXTableGroupOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : XOPTableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : XOPTableGroupOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : EVEXTableOps)
      BaseOp = UnknownOp;

  InitializeBaseTables(Mode);
  InitializeSecondaryTables();
  InitializePrimaryGroupTables();
  InitializeSecondaryGroupTables();
  InitializeSecondaryModRMTables();
  InitializeX87Tables();
  InitializeDDDTables();
  InitializeH0F38Tables();
  InitializeH0F3ATables();
  InitializeVEXTables();
  InitializeXOPTables();
  InitializeEVEXTables();

#ifndef NDEBUG
  auto CheckTable = [&UnknownOp](auto& FinalTable) {
    for (size_t i = 0; i < FinalTable.size(); ++i) {
      auto const &Op = FinalTable.at(i);

      if (Op == UnknownOp) {
        LogMan::Msg::A("Unknown Op: 0x%lx", i);
      }
    }
  };

  // CheckTable(BaseOps);
  // CheckTable(SecondBaseOps);

  // CheckTable(RepModOps);
  // CheckTable(RepNEModOps);
  // CheckTable(OpSizeModOps);
  // CheckTable(X87Ops);

  X86InstDebugInfo::InstallDebugInfo();
  LogMan::Msg::D("X86Tables had %ld total insts, and %ld labeled as understood", Total, NumInsts);
#endif
}

}
