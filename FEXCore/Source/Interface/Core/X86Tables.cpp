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

void InitializeBaseTables(Context::OperatingMode Mode);
void InitializeSecondaryTables(Context::OperatingMode Mode);
void InitializePrimaryGroupTables(Context::OperatingMode Mode);
void InitializeH0F3ATables(Context::OperatingMode Mode);

void InitializeInfoTables(Context::OperatingMode Mode) {
  InitializeBaseTables(Mode);
  InitializeSecondaryTables(Mode);
  InitializePrimaryGroupTables(Mode);
  InitializeH0F3ATables(Mode);
}

} // namespace FEXCore::X86Tables
