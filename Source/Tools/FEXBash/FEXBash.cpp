// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|FEXBash
desc: Wrapper for invoking x86 bash from the rootfs using FEX
$end_info$
*/

#include <FEXCore/fextl/fmt.h>

#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>

static std::string EnchantedPS1(const char* PS1Env) {
  using namespace std::string_view_literals;
  const char* ColorTerm = getenv("COLORTERM");
  const bool SupportsTrueColor = isatty(STDIN_FILENO) && ColorTerm && (ColorTerm == "truecolor"sv || ColorTerm == "24bit"sv);

  std::string PS1 = "PS1=FEXBash ";
  if (SupportsTrueColor) {
    // Rainbow FEXBash text matching the FEX logo
    PS1 = "PS1="
          R"(\[\e[38;2;251;0;145m\]F)"
          R"(\[\e[38;2;199;0;198m\]E)"
          R"(\[\e[38;2;141;68;253m\]X)"
          R"(\[\e[38;2;31;159;248m\]B)"
          R"(\[\e[38;2;0;218;181m\]a)"
          R"(\[\e[38;2;129;229;93m\]s)"
          R"(\[\e[38;2;240;220;10m\]h)"
          R"(\[\e[0m\] )";
  }

  if (PS1Env) {
    PS1 += &PS1Env[strlen("PS1=")];
  } else {
    PS1 += R"(\u@\h:\w> )";
  }

  return PS1;
}

int main(int argc, char** argv, char** const envp) {
  // Skip argv[0]
  const bool EmptyArgs = argc == 1;

  // FEX will handle finding bash in the rootfs
  // Use /bin/bash for interactive mode and /bin/sh when running a script or when using -c
  const char* BashPath = EmptyArgs ? "/bin/bash" : "/bin/sh";

  auto FEXPath = std::filesystem::path(argv[0]).replace_filename("FEX");

  // Check if a local FEX to FEXBash exists
  // If it does then it takes priority over the installed one
  if (!std::filesystem::is_regular_file(FEXPath)) {
    std::error_code ec;
    auto FEXBashPath = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) {
      FEXPath = FEXBashPath.replace_filename("FEX");
    }

    if (!std::filesystem::is_regular_file(FEXPath)) {
      fmt::print(stderr, "Could not locate FEX executable\n");
      std::abort();
    }
  }

  std::vector<const char*> Argv;
  Argv.emplace_back(FEXPath.c_str());
  Argv.emplace_back(BashPath);
  for (int i = 1; i < argc; ++i) {
    Argv.emplace_back(argv[i]);
  }

  // Set --norc when no arguments are passed so PS1 doesn't get overwritten
  if (EmptyArgs) {
    Argv.emplace_back("--norc");
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
  // Keep the string alive until after execve.
  const std::string PS1 = EnchantedPS1(PS1Env);
  Envp.emplace_back(PS1.c_str());
  Envp.emplace_back(nullptr);

  return execve(Argv[0], const_cast<char* const*>(Argv.data()), const_cast<char* const*>(&Envp[0]));
}
