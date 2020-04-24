#pragma once

#include <cassert>
#include <string>
#include <string_view>


namespace FEX::EnvLoader {

  void Load(char *const envp[]);

}