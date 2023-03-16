#pragma once
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/string.h>

#include <string>

/**
 * @brief This is a singleton for storing global configuration state
 */
namespace FEX::Config {
  class EmptyMapper : public FEXCore::Config::Layer {
  public:
    explicit EmptyMapper()
      : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_MAIN) {
    }
    void Load() override {}

  protected:
  };

  void SaveLayerToJSON(const fextl::string& Filename, FEXCore::Config::Layer *const Layer);

  struct ApplicationNames {
    // This is the full path to the program (if it exists).
    fextl::string ProgramPath;
    // This is the program executable name (if it exists).
    fextl::string ProgramName;
  };

  /**
   * @brief Loads the FEX and application configurations for the application that is getting ready to run.
   *
   * @param NoFEXArguments Do we want to parse FEXLoader arguments, Or is this FEXInterpreter?
   * @param LoadProgramConfig Do we want to load application specific configurations?
   * @param argc The `argc` passed to main(...)
   * @param argv The `argv` passed to main(...)
   * @param envp The `envp` passed to main(...)
   * @param ExecFDInterp If FEX was executed with binfmt_misc FD argument
   * @param ProgramFDFromEnv The execveat FD argument passed through FEX
   *
   * @return The application name and path structure
   */
  ApplicationNames LoadConfig(
    bool NoFEXArguments,
    bool LoadProgramConfig,
    int argc,
    char **argv,
    char **const envp,
    bool ExecFDInterp,
    const std::string_view ProgramFDFromEnv
  );
}
