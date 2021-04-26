#include "IRLoader/Loader.h"
#include <FEXCore/Utils/LogManager.h>

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

#include <fstream>

namespace FEX::IRLoader {
  Loader::Loader(std::string const &Filename, std::string const &ConfigFilename) {
    Config.Init(ConfigFilename);
    std::fstream fp(Filename, std::fstream::binary | std::fstream::in);

    if (!fp.is_open()) {
      LogMan::Msg::E("Couldn't open IR file '%s'", Filename.c_str());
      return;
    }

    ParsedCode.reset(FEXCore::IR::Parse(&fp));

    if (ParsedCode) {
      auto NewIR = ParsedCode->ViewIR();
      EntryRIP = 0x40000;
      
      std::stringstream out;
      FEXCore::IR::Dump(&out, &NewIR, nullptr);
      printf("IR:\n%s\n@@@@@\n", out.str().c_str());
    }
  }
}
