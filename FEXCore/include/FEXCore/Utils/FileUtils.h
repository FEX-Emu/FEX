// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/string.h>
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::FileUtils {
/**
 * @brief Removes an entirely directory passed in. Including the path itself.
 *
 * @return True if the directory didn't exist or was deleted.
 */
FEX_DEFAULT_VISIBILITY bool RecursiveRemoveDirectory(const fextl::string& Directory);
} // namespace FEXCore::FileUtils
