#pragma once

#include <cassert>
#include <string>
#include <string_view>


namespace FEX::EnvLoader {
  void Load(char *const envp[]);



#if 0
  class EnvironmentLoader
  {
    virtual ~EnvironmentLoader(){}
    EnvironmentLoader(const char *envp[] {nullptr});

  private:
    unordered_map<string, string> EnvMap;
  };
#endif
}