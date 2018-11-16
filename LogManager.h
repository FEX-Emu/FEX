#pragma once
#include <functional>
#include <sstream>

namespace LogMan {
enum DebugLevels {
  NONE = 0,   ///< Expect zero messages
  ASSERT = 1, ///< Assert throwing
  ERROR = 2,  ///< Only Errors printed
  DEBUG = 3,  ///< Debug messages added
  INFO = 4,   ///< Info messages added
};

constexpr DebugLevels MSG_LEVEL = INFO;

namespace Throw {
using ThrowHandler = std::function<void(std::string const &Message)>;
void InstallHandler(ThrowHandler Handler);

[[noreturn]] void M(std::string const &Message);

static void A(bool Value, std::string const &Message) {
  if (MSG_LEVEL >= ASSERT && !Value)
    M(Message);
}

} // namespace Throw

namespace Msg {
using MsgHandler =
    std::function<void(DebugLevels Level, std::string const &Message)>;
void InstallHandler(MsgHandler Handler);

void M(DebugLevels Level, std::string const &Message);

template <class... Args>
static void A(std::string const &Message, Args... arguments) {
  auto format = [](std::string const &Message, Args... arguments) {
    std::ostringstream Str;
    Str << Message;
    for (auto Arg : {arguments...}) {
      Str << Arg;
    }
    return Str.str();
  };
  if (MSG_LEVEL >= ASSERT)
    M(ERROR, format(Message, arguments...));
  LogMan::Throw::A(false, format(Message, arguments...));
}

template <class... Args>
static void E(std::string const &Message, Args... arguments) {
  auto format = [](std::string const &Message, Args... arguments) {
    std::ostringstream Str;
    Str << Message;
    for (auto Arg : {arguments...}) {
      Str << Arg;
    }
    return Str.str();
  };
  if (MSG_LEVEL >= ERROR)
    M(ERROR, format(Message, arguments...));
}

template <class... Args>
static void D(std::string const &Message, Args... arguments) {
  auto format = [](std::string const &Message, Args... arguments) {
    std::ostringstream Str;
    Str << Message;
    for (auto Arg : {arguments...}) {
      Str << Arg;
    }
    return Str.str();
  };

  if (MSG_LEVEL >= DEBUG)
    M(DEBUG, format(Message, arguments...));
}

template <class... Args>
static void I(std::string const &Message, Args... arguments) {
  auto format = [](std::string const &Message, Args... arguments) {
    std::ostringstream Str;
    Str << Message;
    for (auto Arg : {arguments...}) {
      Str << Arg;
    }
    return Str.str();
  };

  if (MSG_LEVEL >= INFO)
    M(INFO, format(Message, arguments...));
}

} // namespace Msg
} // namespace LogMan
