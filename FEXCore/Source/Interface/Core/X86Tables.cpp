// SPDX-License-Identifier: MIT
/*
$info$
meta: frontend|x86-tables ~ Metadata that drives the frontend x86/64 decoding
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Core/Context.h>

namespace FEXCore::X86Tables {

std::array<X86InstInfo, MAX_PRIMARY_TABLE_SIZE> BaseOps{};
std::array<X86InstInfo, MAX_SECOND_TABLE_SIZE> SecondBaseOps{};
std::array<X86InstInfo, MAX_REP_MOD_TABLE_SIZE> RepModOps{};
std::array<X86InstInfo, MAX_REPNE_MOD_TABLE_SIZE> RepNEModOps{};
std::array<X86InstInfo, MAX_OPSIZE_MOD_TABLE_SIZE> OpSizeModOps{};

std::array<X86InstInfo, MAX_INST_GROUP_TABLE_SIZE> PrimaryInstGroupOps{};
std::array<X86InstInfo, MAX_INST_SECOND_GROUP_TABLE_SIZE> SecondInstGroupOps{};
std::array<X86InstInfo, MAX_SECOND_MODRM_TABLE_SIZE> SecondModRMTableOps{};
std::array<X86InstInfo, MAX_X87_TABLE_SIZE> X87Ops{};
std::array<X86InstInfo, MAX_3DNOW_TABLE_SIZE> DDDNowOps{};
std::array<X86InstInfo, MAX_0F_38_TABLE_SIZE> H0F38TableOps{};
std::array<X86InstInfo, MAX_0F_3A_TABLE_SIZE> H0F3ATableOps{};
std::array<X86InstInfo, MAX_VEX_TABLE_SIZE> VEXTableOps{};
std::array<X86InstInfo, MAX_VEX_GROUP_TABLE_SIZE> VEXTableGroupOps{};
std::array<X86InstInfo, MAX_XOP_TABLE_SIZE> XOPTableOps{};
std::array<X86InstInfo, MAX_XOP_GROUP_TABLE_SIZE> XOPTableGroupOps{};
std::array<X86InstInfo, MAX_EVEX_TABLE_SIZE> EVEXTableOps{};

void InitializeBaseTables(Context::OperatingMode Mode);
void InitializeSecondaryTables(Context::OperatingMode Mode);
void InitializePrimaryGroupTables(Context::OperatingMode Mode);
void InitializeSecondaryGroupTables();
void InitializeSecondaryModRMTables();
void InitializeX87Tables();
void InitializeDDDTables();
void InitializeH0F38Tables();
void InitializeH0F3ATables(Context::OperatingMode Mode);
void InitializeVEXTables();
void InitializeXOPTables();
void InitializeEVEXTables();

#ifndef NDEBUG
uint64_t Total{};
uint64_t NumInsts{};
#endif

void InitializeInfoTables(Context::OperatingMode Mode) {
  InitializeBaseTables(Mode);
  InitializeSecondaryTables(Mode);
  InitializePrimaryGroupTables(Mode);
  InitializeSecondaryGroupTables();
  InitializeSecondaryModRMTables();
  InitializeX87Tables();
  InitializeDDDTables();
  InitializeH0F38Tables();
  InitializeH0F3ATables(Mode);
  InitializeVEXTables();
  InitializeXOPTables();
  InitializeEVEXTables();

#ifndef NDEBUG
  X86InstDebugInfo::InstallDebugInfo();
#endif
}

}
