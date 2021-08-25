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
  FEXCore::Config::AddLayer(FEXCore::Config::CreateMainLayer());
  FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(FEXCore::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

}
