#include <cstring>
#include <sstream>
#include "IRLexer.h"
#include "LogManager.h"
#include "Common/StringUtil.h"

#include <FEXCore/IR/IR.h>

namespace FEX::Debugger::IR {
bool Lexer::Lex(char const *IR) {
//  std::istringstream iss {std::string(IR)};
//  HadError = false;
//
//  [[maybe_unused]] int CurrentLine {};
//  [[maybe_unused]] int CurrentColumn {};
//  while (true) {
//    char Line[256];
//    char *Str = Line;
//    std::string LineStr;
//    std::getline(iss, LineStr);
//    FEX::StringUtil::trim(LineStr);
//    strncpy(Line, LineStr.c_str(), 256);
//
//    struct {
//      bool HadDest;
//      FEXCore::IR::AlignmentType DestLoc;
//    } IRData;
//
//    if (strstr(Str, "%ssa") == nullptr) {
//      IRData.HadDest = true;
//      sscanf(Str, "%%ssa%d =", reinterpret_cast<int*>(&IRData.DestLoc));
//      strtok(Str, " = ");
//      strtok(nullptr, " = ");
//    }
//
//    CurrentColumn = false;
//    ++CurrentLine;
//  }
//  // Our IR is fairly simple to lex, just spin through it
//  return HadError;
  return false;
}
}
