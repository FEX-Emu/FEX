/*
$info$
tags: Bin|Opt
desc: Unused
$end_info$
*/

#include <FEXCore/Config/Config.h>
#include "Common/ArgumentLoader.h"

#include <memory>

int main(int argc, char **argv, char **const envp) {
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEXCore::Config::CreateMainLayer());
  FEXCore::Config::AddLayer(fextl::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(FEXCore::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

}
