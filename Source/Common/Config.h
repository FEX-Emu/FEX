#pragma once

#include "Common/StringConv.h"

#include <cassert>
#include <string>
#include <string_view>

/**
 * @brief This is a singleton for storing global configuration state
 */
namespace FEX::Config {
  void Init();
  void Shutdown();

  void Add(std::string const &Key, std::string_view const Value);
  bool Exists(std::string const &Key);
  std::string_view Get(std::string const &Key);
  std::string_view GetIfExists(std::string const &Key, std::string_view const Default = "");

  template<typename T>
  T Get(std::string const &Key) {
    T Value;
    if (!FEX::StrConv::Conv(Get(Key), &Value)) {
      assert(0 && "Attempted to convert invalid value");
    }
    return Value;
  }

  template<typename T>
  T GetIfExists(std::string const &Key, T Default) {
    T Value;
    if (Exists(Key) && FEX::StrConv::Conv(FEX::Config::Get(Key), &Value)) {
      return Value;
    }
    else {
      return Default;
    }
  }

  template<typename T>
  class Value {
  public:
    Value(std::string const &key, T Default)
      : Key {key}
    {
      ValueData = GetFromConfig(Key, Default);
    }
    void Set(T NewValue) {
      ValueData = NewValue;
      FEX::Config::Add(Key, std::to_string(NewValue));
    }

    T operator()() { return ValueData; }

  private:
    std::string const Key;
    T ValueData;
    T GetFromConfig(std::string const &Key, T Default) {
      return FEX::Config::GetIfExists(Key, Default);
    }
  };

}
