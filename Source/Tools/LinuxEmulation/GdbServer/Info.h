// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|gdbserver
desc: Provides a gdb interface to the guest state
$end_info$
*/
#pragma once

#include <FEXCore/fextl/string.h>

#include <cstdint>
#include <string_view>

namespace FEXCore::X86State {
enum X86Reg : uint32_t;
}

namespace FEX::GDB::Info {
/**
 * @brief Returns textual name of bit location from EFLAGs register.
 *
 * @param Bit Which bit of EFLAG to query
 */
const std::string_view& GetFlagName(unsigned Bit);

/**
 * @brief Returns the textual name of a GPR register
 *
 * @param Reg Index of the register to fetch
 */
std::string_view GetGRegName(unsigned Reg);

/**
 * @brief Fetches the thread's name
 *
 * @param PID The program id of the application
 * @param ThreadID The thread id of the program
 */
fextl::string GetThreadName(uint32_t PID, uint32_t ThreadID);

/**
 * @brief Returns the GDB specific construct of OS describing XML.
 */
fextl::string BuildOSXML();

/**
 * @brief Returns the GDB specific construct of target describing XML.
 */
fextl::string BuildTargetXML();
} // namespace FEX::GDB::Info
