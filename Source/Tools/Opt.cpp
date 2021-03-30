/*
$info$
tags: Bin|Opt
desc: Unused
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"

#include <FEXCore/IR/IntrusiveIRList.h>

int main(int argc, char **argv, char **const envp) {
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::MainLoader>());
  FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::EnvLoader>(envp));
  FEXCore::Config::Load();

}
