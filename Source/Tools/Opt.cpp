#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"

#include <FEXCore/IR/IntrusiveIRList.h>

int main(int argc, char **argv, char **const envp) {
  FEX::Config::Init();
  FEX::EnvLoader::Load(envp);
  FEX::ArgLoader::Load(argc, argv);

  FEX::Config::Shutdown();
}
