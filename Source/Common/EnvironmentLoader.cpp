#include <functional>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include "Common/Config.h"
#include "LogManager.h"

namespace FEX::EnvLoader {

  using string = std::string;
  using string_view = std::string_view;

  void Load(char *const envp[])
  {
    std::unordered_map<string_view,string_view> EnvMap;

    for(const char *const *pvar=envp; pvar && *pvar; pvar++) {
      string_view Var(*pvar);
      size_t pos = Var.rfind('=');
      if (string::npos==pos)
        continue;

      string_view Ident = Var.substr(0,pos);
      string_view Value = Var.substr(pos+1);
      EnvMap[Ident]=Value;
    }

    std::function GetVar = [=](const string_view id)  -> const string_view {
      if (EnvMap.find(id) != EnvMap.end())
        return EnvMap.at(id);

      // If envp[] was empty, search using std::getenv()
      const char* vs = std::getenv(id.data());
      string_view sv(vs?vs:"");
      return sv;
    };

    string_view Value;
    {
      if ((Value = GetVar("FEX_CORE")).size()) {
        // Accept Numeric or name //
        if (isdigit(Value[0])) Config::Add("Core", Value);
        else {
          uint32_t CoreVal = 0;
               if (Value == string_view("irint")) CoreVal = 0; // default
          else if (Value == string_view("irjit")) CoreVal = 1;
#ifdef _M_X86_64
          else if (Value == string_view("host"))  CoreVal = 2;
#endif
          else { LogMan::Msg::D("FEX_CORE has invalid identifier"); }
          Config::Add("Core", std::to_string(CoreVal));
        }
      }

      if ((Value = GetVar("FEX_BREAK")).size()) {
        if (isdigit(Value[0])) Config::Add("Break", Value);
      }

      if ((Value = GetVar("FEX_SINGLE_STEP")).size()) {
        if (isdigit(Value[0])) {
          Config::Add("SingleStep", Value);
          Config::Add("MaxInst", std::to_string(1u));
        }
      }
      else if ((Value = GetVar("FEX_MAX_INST")).size()) {
        if (isdigit(Value[0])) Config::Add("MaxInst", Value);
      }

      if ((Value = GetVar("FEX_MULTIBLOCK")).size()) {
        if (isdigit(Value[0])) Config::Add("Multiblock", Value);
      }

      if ((Value = GetVar("FEX_GDB_SERVER")).size()) {
        if (isdigit(Value[0])) Config::Add("GdbServer", Value);
      }

      if ((Value = GetVar("FEX_THREADS")).size()) {
        if (isdigit(Value[0])) Config::Add("Threads", Value);
      }

      if ((Value = GetVar("FEX_TSO_ENABLED")).size()) {
        if (isdigit(Value[0])) Config::Add("TSOEnabled", Value);
      }

      if ((Value = GetVar("FEX_SMC_CHECKS")).size()) {
        if (isdigit(Value[0])) Config::Add("SMCChecks", Value);
      }
    }

    {
      if ((Value = GetVar("FEX_ROOTFS")).size()) {
        Config::Add("RootFS", Value);
        if (!std::filesystem::exists(Value)) {
          LogMan::Msg::D("FEX_ROOTFS '%s' doesn't exist", Value.data());
          Config::Add("RootFS", "");
        }
      }

      if ((Value = GetVar("FEX_UNIFIED_MEM")).size()) {
        if (isdigit(Value[0])) Config::Add("UnifiedMemory", Value);
      }
    }

    {
      if ((Value = GetVar("FEX_DUMP_GPRS")).size()) {
        if (isdigit(Value[0])) Config::Add("DumpGPRs", Value);
      }

      if ((Value = GetVar("FEX_IPC_CLIENT")).size()) {
        if (isdigit(Value[0])) Config::Add("IPCClient", Value);
      }

      if ((Value = GetVar("FEX_ELF_TYPE")).size()) {
        if (isdigit(Value[0])) Config::Add("ELFType", Value);
      }

      if ((Value = GetVar("FEX_IPCID")).size()) {
        Config::Add("IPCID", Value);
      }
    }

    {
      if ((Value = GetVar("FEX_SILENTLOG")).size()) {
        Config::Add("SilentLog", Value);
      }

      if ((Value = GetVar("FEX_OUTPUTLOG")).size()) {
        Config::Add("OutputLog", Value);
      }
    }
  }

}
