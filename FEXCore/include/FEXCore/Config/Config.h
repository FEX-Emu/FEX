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
#include <variant>

namespace FEXCore::Config {
namespace Handler {
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
static inline std::optional<fextl::string> EnumParser(const ArrayPairType& EnumPairs, const std::string_view View) {
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
    using StringArrayType = fextl::list<fextl::string>;
#define OPT_BASE(type, group, enum, json, default) using P(enum) = P(type);
#define OPT_STR(group, enum, json, default) using P(enum) = fextl::string;
#define OPT_STRARRAY(group, enum, json, default) using P(enum) = StringArrayType;
#include <FEXCore/Config/ConfigValues.inl>
  } // namespace Type
#define FEX_CONFIG_OPT(name, enum)                                          \
  FEXCore::Config::Value<FEXCore::Config::DefaultValues::Type::enum> name { \
    FEXCore::Config::CONFIG_##enum,                                         \
    FEXCore::Config::DefaultValues::enum                                    \
  }

#undef P
} // namespace DefaultValues

FEX_DEFAULT_VISIBILITY void SetDataDirectory(std::string_view Path, bool Global);
FEX_DEFAULT_VISIBILITY void SetConfigDirectory(const std::string_view Path, bool Global);
FEX_DEFAULT_VISIBILITY void SetConfigFileLocation(std::string_view Path, bool Global);

FEX_DEFAULT_VISIBILITY const fextl::string& GetDataDirectory(bool Global = false);
FEX_DEFAULT_VISIBILITY const fextl::string& GetConfigDirectory(bool Global);
FEX_DEFAULT_VISIBILITY const fextl::string& GetConfigFileLocation(bool Global = false);
FEX_DEFAULT_VISIBILITY fextl::string GetApplicationConfig(const std::string_view Program, bool Global);

using LayerValue =
  std::variant< fextl::string, DefaultValues::Type::StringArrayType, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, bool >;

using LayerOptions = fextl::unordered_map<ConfigOption, LayerValue>;

class FEX_DEFAULT_VISIBILITY Layer {
public:
  explicit Layer(const LayerType _Type);
  virtual ~Layer();

  virtual void Load() = 0;

  bool OptionExists(ConfigOption Option) const {
    return OptionMap.find(Option) != OptionMap.end();
  }

  std::optional<DefaultValues::Type::StringArrayType*> All(ConfigOption Option) {
    const auto it = OptionMap.find(Option);
    if (it == OptionMap.end()) {
      return std::nullopt;
    }

    auto& Value = it->second;
    LOGMAN_THROW_A_FMT(std::holds_alternative<DefaultValues::Type::StringArrayType>(Value), "Tried to get config of invalid type!");

    return &std::get<DefaultValues::Type::StringArrayType>(Value);
  }

  std::optional<fextl::string*> Get(ConfigOption Option) {
    const auto it = OptionMap.find(Option);
    if (it == OptionMap.end()) {
      return std::nullopt;
    }

    auto& Value = it->second;
    LOGMAN_THROW_A_FMT(std::holds_alternative<fextl::string>(Value), "Tried to get config of invalid type!");

    return &std::get<fextl::string>(Value);
  }

  // Set will overwrite the object with a fextl::string without tests.
  void Set(ConfigOption Option, const char* Data) {
    LOGMAN_THROW_A_FMT(Data != nullptr, "Data can't be null");
    OptionMap[Option].emplace<fextl::string>(fextl::string(Data));
  }

  void Set(ConfigOption Option, std::string_view Data) {
    OptionMap[Option].emplace<fextl::string>(fextl::string(Data));
  }

  void Set(ConfigOption Option, fextl::string Data) {
    OptionMap[Option].emplace<fextl::string>(std::move(Data));
  }

  void Set(ConfigOption Option, std::optional<fextl::string> Data) {
    if (Data) {
      OptionMap[Option].emplace<fextl::string>(std::move(*Data));
    }
  }

  // AppendStrArrayValue will append strings to its StringArrayType.
  // If the value was previously a different type, then throw an assert.
  void AppendStrArrayValue(ConfigOption Option, std::string_view Data) {
    auto it = OptionMap.find(Option);
    if (it == OptionMap.end()) {
      // If the option didn't exist as a StringArrayType yet, emplace it.
      it = OptionMap.emplace(Option, DefaultValues::Type::StringArrayType {}).first;
    }

    auto& Value = it->second;
    LOGMAN_THROW_A_FMT(std::holds_alternative<DefaultValues::Type::StringArrayType>(Value), "Tried to get config of invalid type!");
    std::get<DefaultValues::Type::StringArrayType>(Value).emplace_back(Data);
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
FEX_DEFAULT_VISIBILITY std::optional<DefaultValues::Type::StringArrayType*> All(ConfigOption Option);
FEX_DEFAULT_VISIBILITY std::optional<fextl::string*> Get(ConfigOption Option);
FEX_DEFAULT_VISIBILITY void Set(ConfigOption Option, std::string_view Data);
FEX_DEFAULT_VISIBILITY void Erase(ConfigOption Option);

template<typename T>
class FEX_DEFAULT_VISIBILITY Value {
public:
  // Single value type.
  template<typename TT = T>
  requires (std::is_fundamental_v<TT> || std::is_same_v<TT, fextl::string>)
  Value(FEXCore::Config::ConfigOption Option, TT Default) {
    ValueData = GetIfExists(Option, Default);
  }

  template<typename TT = T>
  requires (std::is_fundamental_v<TT> || std::is_same_v<TT, fextl::string>)
  Value(FEXCore::Config::ConfigOption Option, std::string_view Default) {
    ValueData = GetIfExists(Option, Default);
  }

  operator T() const {
    return ValueData;
  }

  T operator()() const requires (std::is_fundamental_v<T>)
  {
    return ValueData;
  }

  const fextl::string& operator()() const requires (std::is_same_v<T, fextl::string>)
  {
    return ValueData;
  }

  Value(T Value) requires (!std::is_same_v<T, DefaultValues::Type::StringArrayType>)
  {
    ValueData = std::move(Value);
  }

  // Array value types.
  Value(FEXCore::Config::ConfigOption Option, std::string_view) requires (std::is_same_v<T, DefaultValues::Type::StringArrayType>)
  {
    GetListIfExists(Option, &ValueData);
  }

  DefaultValues::Type::StringArrayType& All() requires (std::is_same_v<T, DefaultValues::Type::StringArrayType>)
  {
    return ValueData;
  }

private:
  T ValueData {};

  static T GetIfExists(FEXCore::Config::ConfigOption Option, T Default);
  static T GetIfExists(FEXCore::Config::ConfigOption Option, std::string_view Default);

  static void GetListIfExists(FEXCore::Config::ConfigOption Option, DefaultValues::Type::StringArrayType* List);
};
} // namespace FEXCore::Config
