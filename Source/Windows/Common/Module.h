// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <FEXCore/fextl/string.h>
#include <winternl.h>

namespace FEX::Windows {
fextl::string GetExecutableFilePath() {
  std::array<WCHAR, PATH_MAX> Buf;
  UNICODE_STRING PathW {.Length = 0, .MaximumLength = Buf.size() * sizeof(WCHAR), .Buffer = Buf.data()};

  if (LdrGetDllFullName(nullptr, &PathW)) {
    return {};
  }

  STRING PathA;
  RtlUnicodeStringToAnsiString(&PathA, &PathW, TRUE);
  fextl::string Path(PathA.Buffer);
  RtlFreeAnsiString(&PathA);

  return Path.substr(Path.find_last_of('\\') + 1);
}
} // namespace FEX::Windows
