#pragma once

#include "Common/StringConv.h"

#include <cassert>
#include <list>
#include <string>
#include <string_view>

/**
 * @brief This is a singleton for storing global configuration state
 */
namespace FEX::Config {
  template<typename T>
  class Value;

  void Init();
  void Shutdown();

  void Add(std::string const &Key, std::string_view const Value);
  void Append(std::string const &Key, std::string_view const Value);
  bool Exists(std::string const &Key);
  Value<std::string> &Get(std::string const &Key);
  Value<std::string> *GetIfExists(std::string const &Key);

  template<typename T>
  T Get(std::string const &Key);

  template<typename T>
  T GetIfExists(std::string const &Key, T Default);

  void GetListIfExists(std::string const &Key, std::list<std::string> *List);

  template<typename T>
  class Value {
  public:
    Value(std::string const &key, std::string_view const Value, bool ConfigBacking)
      : Key {key} {
      ValueData = Value;
    }

    template <typename TT = T,
      typename std::enable_if<!std::is_same<TT, std::string>::value, int>::type = 0>
    Value(std::string const &key, T Default)
      : Key {key} {
      ValueData = FEX::Config::GetIfExists<T>(Key, Default);
    }

    template <typename TT = T,
      typename std::enable_if<std::is_same<TT, std::string>::value, int>::type = 0>
    Value(std::string const &key, T Default)
      : Key {key} {
      ValueData = FEX::Config::GetIfExists<T>(Key, Default);
      FEX::Config::GetListIfExists(Key, &AppendList);
    }

    void Set(T NewValue) {
      ValueData = NewValue;
      FEX::Config::Add(Key, std::to_string(NewValue));
    }

    void Append(T NewValue) {
      AppendList.emplace_back(NewValue);
    }

    T operator()() { return ValueData; }
    std::list<T> &All() { return AppendList; }

  private:
    std::string const Key;
    T ValueData;
    std::list<T> AppendList;
  };


}
