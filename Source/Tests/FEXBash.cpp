/*
$info$
tags: Bin|FEXBash
desc: Launches bash under FEX and passes arguments via -c to it
$end_info$
*/

#include "ConfigDefines.h"
#include "Common/ArgumentLoader.h"
#include "Common/RootFSSetup.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/LogManager.h>

#include <filesystem>
#include <iterator>
#include <memory>
#include <stddef.h>
#include <string>
#include <unistd.h>
#include <vector>

int main(int argc, char **argv, char **const envp) {
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEXCore::Config::CreateMainLayer());
  FEX::ArgLoader::LoadWithoutArguments(argc, argv);
  FEXCore::Config::AddLayer(FEXCore::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();

  auto Args = FEX::ArgLoader::Get();

  // Ensure RootFS is setup before config options try to pull CONFIG_ROOTFS
  if (!FEX::RootFS::Setup(envp)) {
    fprintf(stderr, "RootFS configuration failed.");
    return -1;
  }

  FEX_CONFIG_OPT(RootFSPath, ROOTFS);
  std::vector<const char*> Argv;
  std::string BinShPath = RootFSPath() + "/bin/sh";
  std::string BinBashPath = RootFSPath() + "/bin/bash";

  std::string FEXInterpreterPath = std::filesystem::path(argv[0]).parent_path().string() + "FEXInterpreter";
  // Check if a local FEXInterpreter to FEXBash exists
  // If it does then it takes priority over the installed one
  if (!std::filesystem::exists(FEXInterpreterPath)) {
    FEXInterpreterPath = FEXINTERPRETER_PATH;
  }
  const char *FEXArgs[] = {
    FEXInterpreterPath.c_str(),
    Args.empty() ? BinBashPath.c_str() : BinShPath.c_str(),
    "-c",
  };

  // Remove -c argument if arguments are empty
  // Lets us start an emulated bash instance
  const size_t FEXArgsCount = std::size(FEXArgs) - (Args.empty() ? 1 : 0);

  Argv.resize(Args.size() + FEXArgsCount + 1);

  // Pass in the FEXInterpreter arguments
  for (size_t i = 0; i < FEXArgsCount; ++i) {
    Argv[i] = FEXArgs[i];
  }

  // Bring in passed in arguments
  for (size_t i = 0; i < Args.size(); ++i) {
    Argv[i + FEXArgsCount] = Args[i].c_str();
  }
  Argv[Argv.size() - 1] = nullptr;

  return execve(Argv[0], const_cast<char *const*>(&Argv.at(0)), envp);
}
