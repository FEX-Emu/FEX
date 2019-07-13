#include "Common/ArgumentLoader.h"
#include "Common/Config.h"

#include <FEXCore/IR/IntrusiveIRList.h>

int main(int argc, char **argv) {
  FEX::Config::Init();
  FEX::ArgLoader::Load(argc, argv);

  FEX::Config::Shutdown();
}
