#include "ArgumentLoader.h"
#include "OptionParser.h"

#include "git_version.h"

#include <fmt/format.h>

namespace FEXServer::Config {
  static std::string Version = "FEX-Emu (" GIT_DESCRIBE_STRING ") ";

  FEXServerOptions Load(int argc, char **argv) {
    FEXServerOptions FEXOptions{};
    optparse::OptionParser Parser = optparse::OptionParser()
      .version(Version);

    Parser.add_option("-k", "--kill")
      .action("store_true")
      .set_default(false)
      .help("Shutdown an already active FEXServer");

    Parser.add_option("-f", "--foreground")
      .action("store_true")
      .set_default(false)
      .help("Run this FEXServer in the foreground");

    Parser.add_option("-p", "--persistent")
      .action("store")
      .type("int")
      .set_default(0)
      .set_optional_value(true)
      .metavar("n")
      .help("Make FEXServer persistent. Optional number of seconds");

    Parser.add_option("-v")
      .action("version")
      .help("Version string");

    optparse::Values Options = Parser.parse_args(argc, argv);

    FEXOptions.Kill = Options.get("kill");
    FEXOptions.Foreground = Options.get("foreground");
    FEXOptions.PersistentTimeout = Options.get("persistent");

    return FEXOptions;
  }
}
