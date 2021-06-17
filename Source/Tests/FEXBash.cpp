/*
$info$
tags: Bin|FEXBash
desc: Launches bash under FEX and passes arguments via -c to it
$end_info$
*/

#include "ConfigDefines.h"
#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"
#include "Common/RootFSSetup.h"

#include <FEXCore/Config/Config.h>
#include <filesystem>
#include <string>
#include <unistd.h>

int main(int argc, char **argv, char **const envp) {
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::MainLoader>());
  FEX::ArgLoader::LoadWithoutArguments(argc, argv);
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::EnvLoader>(envp));
  FEXCore::Config::Load();

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();

  auto Args = FEX::ArgLoader::Get();

  if (Args.empty()) {
    // Early exit if we weren't passed an argument
    return 0;
  }

  // Ensure RootFS is setup before config options try to pull CONFIG_ROOTFS
  if (!FEX::RootFS::Setup(envp)) {
    LogMan::Msg::E("RootFS failure");
    return -1;
  }

  FEX_CONFIG_OPT(RootFSPath, ROOTFS);
  std::vector<const char*> Argv;
  std::string BinShPath = RootFSPath() + "/bin/sh";
  std::string FEXInterpreterPath = std::filesystem::path(argv[0]).parent_path().string() + "FEXInterpreter";
  // Check if a local FEXInterpreter to FEXBash exists
  // If it does then it takes priority over the installed one
  if (!std::filesystem::exists(FEXInterpreterPath)) {
    FEXInterpreterPath = FEXINTERPRETER_PATH;
  }
  const char *FEXArgs[] = {
    FEXInterpreterPath.c_str(),
    BinShPath.c_str(),
    "-c",
  };
  constexpr size_t FEXArgsCount = std::size(FEXArgs);

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
