// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/EnumOperators.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>

#include <charconv>
#include <optional>
#include <stdint.h>

namespace FEXCore::Config {
namespace Handler {
  static inline std::optional<fextl::string> CoreHandler(std::string_view Value) {
    if (Value == "irjit") {
      return "0";
    }
#ifdef _M_X86_64
    else if (Value == "host") {
      return "1";
    }
#endif
    return "0";
  }

  static inline std::optional<fextl::string> SMCCheckHandler(std::string_view Value) {
    if (Value == "none") {
      return "0";
    } else if (Value == "mtrack") {
      return "1";
    } else if (Value == "full") {
      return "2";
    }
    return "0";
  }
  static inline std::optional<fextl::string> CacheObjectCodeHandler(std::string_view Value) {
    if (Value == "none") {
      return "0";
    } else if (Value == "read") {
      return "1";
    } else if (Value == "write") {
      return "2";
    }
    return "0";
  }
} // namespace Handler

enum ConfigOption {
#define OPT_BASE(type, group, enum, json, default) CONFIG_##enum,
#include <FEXCore/Config/ConfigValues.inl>
};

#define ENUMDEFINES
#include <FEXCore/Config/ConfigOptions.inl>

enum ConfigCore {
  CONFIG_IRJIT,
  CONFIG_CUSTOM,
};

enum ConfigSMCChecks {
  CONFIG_SMC_NONE,
  CONFIG_SMC_MTRACK,
  CONFIG_SMC_FULL,
};

enum ConfigObjectCodeHandler {
  CONFIG_NONE,
  CONFIG_READ,
  CONFIG_READWRITE,
};

enum class LayerType {
  LAYER_GLOBAL_MAIN, ///< /usr/share/fex-emu/Config.json by default
  LAYER_MAIN,
  LAYER_ARGUMENTS,
  LAYER_GLOBAL_STEAM_APP,
  LAYER_GLOBAL_APP,
  LAYER_LOCAL_STEAM_APP,
  LAYER_LOCAL_APP,
  LAYER_USER_OVERRIDE,
  LAYER_ENVIRONMENT,
  LAYER_TOP,
};

template<typename PairTypes, typename ArrayPairType>
static inline std::optional<fextl::string> EnumParser(const ArrayPairType& EnumPairs, std::string_view const View) {
  uint64_t EnumMask {};
  auto Results = std::from_chars(View.data(), View.data() + View.size(), EnumMask);
  if (Results.ec == std::errc()) {
    // If the data is a valid number, just pass it through.
    return std::nullopt;
  }

  auto Begin = 0;
  auto End = View.find_first_of(',');
  std::string_view Option = View.substr(Begin, End);
  while (Option.size() != 0) {
    auto EnumValue =
      std::find_if(EnumPairs.begin(), EnumPairs.end(), [Option](const PairTypes& Value) -> bool { return Value.first == Option; });

    if (EnumValue == EnumPairs.end()) {
      LogMan::Msg::IFmt("Skipping Unknown option: {}", Option);
    } else {
      EnumMask |= FEXCore::ToUnderlying(EnumValue->second);
    }

    if (End == std::string::npos) {
      break;
    }
    Begin = End + 1;
    End = View.find_first_of(',', Begin);
    Option = View.substr(Begin, End - Begin);
  }

  return fextl::fmt::format("{}", EnumMask);
}

namespace DefaultValues {
#define P(x) x
#define OPT_BASE(type, group, enum, json, default) extern const P(type) P(enum);
#define OPT_STR(group, enum, json, default) extern const std::string_view P(enum);
#define OPT_STRARRAY(group, enum, json, default) OPT_STR(group, enum, json, default)
#include <FEXCore/Config/ConfigValues.inl>

  namespace Type {
#define OPT_BASE(type, group, enum, json, default) using P(enum) = P(type);
#define OPT_STR(group, enum, json, default) using P(enum) = fextl::string;
#define OPT_STRARRAY(group, enum, json, default) OPT_STR(group, enum, json, default)
#include <FEXCore/Config/ConfigValues.inl>
  } // namespace Type
#define FEX_CONFIG_OPT(name, enum)                                          \
  FEXCore::Config::Value<FEXCore::Config::DefaultValues::Type::enum> name { \
    FEXCore::Config::CONFIG_##enum,                                         \
    FEXCore::Config::DefaultValues::enum                                    \
  }

#undef P
} // namespace DefaultValues

FEX_DEFAULT_VISIBILITY void SetDataDirectory(std::string_view Path);
FEX_DEFAULT_VISIBILITY void SetConfigDirectory(const std::string_view Path, bool Global);
FEX_DEFAULT_VISIBILITY void SetConfigFileLocation(std::string_view Path, bool Global);

FEX_DEFAULT_VISIBILITY const fextl::string& GetDataDirectory();
FEX_DEFAULT_VISIBILITY const fextl::string& GetConfigDirectory(bool Global);
FEX_DEFAULT_VISIBILITY const fextl::string& GetConfigFileLocation(bool Global = false);
FEX_DEFAULT_VISIBILITY fextl::string GetApplicationConfig(const std::string_view Program, bool Global);

using LayerValue = fextl::list<fextl::string>;
using LayerOptions = fextl::unordered_map<ConfigOption, LayerValue>;

class FEX_DEFAULT_VISIBILITY Layer {
public:
  explicit Layer(const LayerType _Type);
  virtual ~Layer();

  virtual void Load() = 0;

  bool OptionExists(ConfigOption Option) const {
    return OptionMap.find(Option) != OptionMap.end();
  }

  std::optional<LayerValue*> All(ConfigOption Option) {
    const auto it = OptionMap.find(Option);
    if (it == OptionMap.end()) {
      return std::nullopt;
    }

    return &it->second;
  }

  std::optional<fextl::string*> Get(ConfigOption Option) {
    const auto it = OptionMap.find(Option);
    if (it == OptionMap.end()) {
      return std::nullopt;
    }

    return &it->second.front();
  }

  void Set(ConfigOption Option, const char* Data) {
    LOGMAN_THROW_AA_FMT(Data != nullptr, "Data can't be null");
    OptionMap[Option].emplace_back(fextl::string(Data));
  }

  void Set(ConfigOption Option, std::string_view Data) {
    OptionMap[Option].emplace_back(fextl::string(Data));
  }

  void Set(ConfigOption Option, fextl::string Data) {
    OptionMap[Option].emplace_back(std::move(Data));
  }

  void Set(ConfigOption Option, std::optional<fextl::string> Data) {
    if (Data) {
      OptionMap[Option].emplace_back(std::move(*Data));
    }
  }

  void EraseSet(ConfigOption Option, std::string_view Data) {
    Erase(Option);
    Set(Option, Data);
  }

  void Erase(ConfigOption Option) {
    OptionMap.erase(Option);
  }

  LayerType GetLayerType() const {
    return Type;
  }
  const LayerOptions& GetOptionMap() const {
    return OptionMap;
  }

protected:
  const LayerType Type;
  LayerOptions OptionMap;
};

FEX_DEFAULT_VISIBILITY void Initialize();
FEX_DEFAULT_VISIBILITY void Shutdown();

FEX_DEFAULT_VISIBILITY void Load();
FEX_DEFAULT_VISIBILITY void ReloadMetaLayer();
FEX_DEFAULT_VISIBILITY fextl::string FindContainer();
FEX_DEFAULT_VISIBILITY fextl::string FindContainerPrefix();

FEX_DEFAULT_VISIBILITY void AddLayer(fextl::unique_ptr<FEXCore::Config::Layer> _Layer);

FEX_DEFAULT_VISIBILITY bool Exists(ConfigOption Option);
FEX_DEFAULT_VISIBILITY std::optional<LayerValue*> All(ConfigOption Option);
FEX_DEFAULT_VISIBILITY std::optional<fextl::string*> Get(ConfigOption Option);

FEX_DEFAULT_VISIBILITY void Set(ConfigOption Option, std::string_view Data);
FEX_DEFAULT_VISIBILITY void Erase(ConfigOption Option);
FEX_DEFAULT_VISIBILITY void EraseSet(ConfigOption Option, std::string_view Data);

template<typename T>
class FEX_DEFAULT_VISIBILITY Value {
public:
  template<typename TT = T>
  requires (!std::is_same_v<TT, fextl::string>)
  Value(FEXCore::Config::ConfigOption _Option, TT Default)
    : Option {_Option} {
    ValueData = GetIfExists(Option, Default);
  }

  template<typename TT = T>
  requires (std::is_same_v<TT, fextl::string>)
  Value(FEXCore::Config::ConfigOption _Option, TT Default)
    : Option {_Option} {
    ValueData = GetIfExists(Option, Default);
    GetListIfExists(Option, &AppendList);
  }

  template<typename TT = T>
  requires (std::is_same_v<TT, fextl::string>)
  Value(FEXCore::Config::ConfigOption _Option, std::string_view Default)
    : Option {_Option} {
    ValueData = GetIfExists(Option, Default);
    GetListIfExists(Option, &AppendList);
  }

  template<typename TT = T>
  requires (!std::is_same_v<TT, fextl::string>)
  Value(FEXCore::Config::ConfigOption _Option)
    : Option {_Option} {
    if (!FEXCore::Config::Exists(Option)) {
      ERROR_AND_DIE_FMT("FEXCore::Config::Value has no value");
    }

    ValueData = Get(Option);
  }

  template<typename TT = T>
  requires (std::is_same_v<TT, fextl::string>)
  Value(FEXCore::Config::ConfigOption _Option)
    : Option {_Option} {
    if (!FEXCore::Config::Exists(Option)) {
      ERROR_AND_DIE_FMT("FEXCore::Config::Value has no value");
    }

    ValueData = GetIfExists(Option);
    GetListIfExists(Option, &AppendList);
  }

  operator T() const {
    return ValueData;
  }

  template<typename TT = T>
  requires (!std::is_same_v<TT, fextl::string>)
  T operator()() const {
    return ValueData;
  }

  template<typename TT = T>
  requires (std::is_same_v<TT, fextl::string>)
  const T& operator()() const {
    return ValueData;
  }

  Value<T>(T Value) {
    ValueData = std::move(Value);
  }
  fextl::list<T>& All() {
    return AppendList;
  }

private:
  FEXCore::Config::ConfigOption Option;
  T ValueData;
  fextl::list<T> AppendList;

  static T Get(FEXCore::Config::ConfigOption Option);
  static T GetIfExists(FEXCore::Config::ConfigOption Option, T Default);
  static T GetIfExists(FEXCore::Config::ConfigOption Option, std::string_view Default);

  static void GetListIfExists(FEXCore::Config::ConfigOption Option, fextl::list<fextl::string>* List);
};
} // namespace FEXCore::Config
