// SPDX-License-Identifier: MIT
#include "ArgumentLoader.h"
#include "Common/MicroArgParser.h"
#include <FEXCore/fextl/fmt.h>

#include "git_version.h"

namespace FEXServer::Config {

FEXServerOptions Load(int argc, char** argv) {
  static std::array<FEX::MicroArgParser::ParseMember, 6> Args = {{
    {FEX::MicroArgParser::ParseMember::Type::Bool, "-k", "--kill", "Shutdown an already active FEXServer", "0"},
    {FEX::MicroArgParser::ParseMember::Type::Bool, "-f", "--foreground", "Run this FEXServer in the foreground", "0"},
    {FEX::MicroArgParser::ParseMember::Type::IntOptional, "-p", "--persistent", "Make FEXServer persistent. Optional number of seconds.", "0"},
    {FEX::MicroArgParser::ParseMember::Type::Bool, "-w", "--wait", "Wait for the FEXServer to shutdown", "0"},
    // Defaults
    {FEX::MicroArgParser::ParseMember::Type::Bool, "-v", "--version", "show program's verison number and exit", "0"},
    {FEX::MicroArgParser::ParseMember::Type::Bool, "-h", "--help", "show this help message and exit", "0"},
  }};

  FEX::MicroArgParser Parser(Args);
  Parser.Version("FEX-Emu (" GIT_DESCRIBE_STRING ") ");
  Parser.Parse(argc, argv);

  FEXServerOptions FEXOptions {};
  FEXOptions.Kill = Parser.Get<bool>("-k");
  FEXOptions.Foreground = Parser.Get<bool>("-f");
  FEXOptions.Wait = Parser.Get<bool>("-w");
  if (FEXOptions.Wait) {
    FEXOptions.Foreground = true;
  }

  FEXOptions.PersistentTimeout = Parser.Get<uint32_t>("-p");

  return FEXOptions;
}
} // namespace FEXServer::Config
