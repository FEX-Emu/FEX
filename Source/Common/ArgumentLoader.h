#pragma once

#include <string>
#include <vector>

namespace FEX::ArgLoader {
  void Load(int argc, char **argv);

  std::vector<std::string> Get();
  std::vector<std::string> GetParsedArgs();

}
