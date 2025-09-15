// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|FEXBash
desc: Launches bash under FEX and passes arguments via -c to it
$end_info$
*/

#include <FEXCore/fextl/fmt.h>

#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>

int main(int argc, char** argv, char** const envp) {
  // Skip argv[0].
  const int ArgCount = argc - 1;
  const bool EmptyArgs = ArgCount == 0;

  std::vector<const char*> Argv;
  // FEX will handle finding bash in the rootfs
  // Use /bin/sh for -c commands and /bin/bash for interactive mode
  const char* BashPath = EmptyArgs ? "/bin/bash" : "/bin/sh";

  std::string FEXPath = std::filesystem::path(argv[0]).parent_path().string() + "/FEX";

  // Check if a local FEX to FEXBash exists
  // If it does then it takes priority over the installed one
  if (!std::filesystem::exists(FEXPath)) {
    char FEXBashPath[PATH_MAX];
    auto Result = readlink("/proc/self/exe", FEXBashPath, PATH_MAX);
    if (Result != -1) {
      FEXPath = std::filesystem::path(&FEXBashPath[0], &FEXBashPath[Result]).parent_path().string() + "/FEX";
    }

    if (!std::filesystem::exists(FEXPath)) {
      fmt::print(stderr, "Could not locate FEX executable\n");
      std::abort();
    }
  }
  const char* FEXArgs[] = {
    FEXPath.c_str(),
    BashPath,
    "-c",
  };

  // Remove -c argument if arguments are empty
  // Lets us start an emulated bash instance
  const size_t FEXArgsCount = std::size(FEXArgs) - (EmptyArgs ? 1 : 0);

  Argv.resize(ArgCount + FEXArgsCount);

  // Pass in the FEX arguments
  for (size_t i = 0; i < FEXArgsCount; ++i) {
    Argv[i] = FEXArgs[i];
  }

  // Bring in passed in arguments
  for (size_t i = 0; i < ArgCount; ++i) {
    Argv[i + FEXArgsCount] = argv[i + 1];
  }

  // Set --norc when no arguments are passed so PS1 doesn't get overwritten
  const char* NoRC = "--norc";
  if (EmptyArgs) {
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

  return execve(Argv[0], const_cast<char* const*>(Argv.data()), const_cast<char* const*>(&Envp[0]));
}
