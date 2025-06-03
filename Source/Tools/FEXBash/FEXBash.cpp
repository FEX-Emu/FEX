// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|FEXBash
desc: Launches bash under FEX and passes arguments via -c to it
$end_info$
*/

#include "ConfigDefines.h"
#include "Common/ArgumentLoader.h"

#include <FEXCore/Config/Config.h>

#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>

int main(int argc, char** argv, char** const envp) {
  FEX::ArgLoader::ArgLoader ArgsLoader(FEX::ArgLoader::ArgLoader::LoadType::WITHOUT_FEXLOADER_PARSER, argc, argv);
  auto Args = ArgsLoader.Get();

  std::vector<const char*> Argv;
  // FEXInterpreter will handle finding bash in the rootfs
  // Use /bin/sh for -c commands and /bin/bash for interactive mode
  const char* BashPath = Args.empty() ? "/bin/bash" : "/bin/sh";

  std::string FEXInterpreterPath = std::filesystem::path(argv[0]).parent_path().string() + "FEXInterpreter";
  // Check if a local FEXInterpreter to FEXBash exists
  // If it does then it takes priority over the installed one
  if (!std::filesystem::exists(FEXInterpreterPath)) {
    FEXInterpreterPath = FEXCore::Config::FindContainerPrefix() + FEXINTERPRETER_PATH;
  }
  const char* FEXArgs[] = {
    FEXInterpreterPath.c_str(),
    BashPath,
    "-c",
  };

  // Remove -c argument if arguments are empty
  // Lets us start an emulated bash instance
  const size_t FEXArgsCount = std::size(FEXArgs) - (Args.empty() ? 1 : 0);

  Argv.resize(Args.size() + FEXArgsCount);

  // Pass in the FEXInterpreter arguments
  for (size_t i = 0; i < FEXArgsCount; ++i) {
    Argv[i] = FEXArgs[i];
  }

  // Bring in passed in arguments
  for (size_t i = 0; i < Args.size(); ++i) {
    Argv[i + FEXArgsCount] = Args[i].c_str();
  }

  // Set --norc when no arguments are passed so PS1 doesn't get overwritten
  const char* NoRC = "--norc";
  if (Args.empty()) {
    Argv.emplace_back(NoRC);
  }

  Argv.emplace_back(nullptr);

  // Prepend `FEXBash>` to PS1 to be less confusing about running under emulation
  // In most cases PS1 isn't an environment variable, but instead a shell variable
  // But in case the user has set the PS1 environment variable then still prepend
  //
  // To get the shell variables as an environment variable then you can do `PS1=$PS1 FEXBash`
  std::vector<const char*> Envp {};
  char* PS1Env {};
  for (unsigned i = 0;; ++i) {
    if (envp[i] == nullptr) {
      break;
    }
    if (strstr(envp[i], "PS1=") == envp[i]) {
      PS1Env = envp[i];
    } else {
      Envp.emplace_back(envp[i]);
    }
  }

  std::string PS1 = "PS1=FEXBash-\\u@\\h:\\w> ";
  if (PS1Env) {
    PS1 += &PS1Env[strlen("PS1=")];
  }
  Envp.emplace_back(PS1.c_str());
  Envp.emplace_back(nullptr);

  return execve(Argv[0], const_cast<char* const*>(&Argv.at(0)), const_cast<char* const*>(&Envp[0]));
}
