/*
$info$
tags: Bin|Opt
desc: Unused
$end_info$
*/

#include <FEXCore/Config/Config.h>
#include "Common/Config.h"
#include "Common/ArgumentLoader.h"

#include <memory>

int main(int argc, char **argv, char **const envp) {
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEX::Config::CreateMainLayer());
  FEXCore::Config::AddLayer(fextl::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(FEX::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

}
