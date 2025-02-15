// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/string.h>

namespace FEX::ArgLoader {
class ArgLoader;
}
/**
 * @brief This is a singleton for storing global configuration state
 */
namespace FEX::Config {
class EmptyMapper : public FEXCore::Config::Layer {
public:
  explicit EmptyMapper()
    : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_MAIN) {}
  void Load() override {}

protected:
};

void SaveLayerToJSON(const fextl::string& Filename, const FEXCore::Config::Layer* Layer);
void SaveLayerToJSON(const fextl::string& Filename, const FEXCore::Config::Layer* Layer, const fextl::unordered_map<fextl::string, bool>& HostLibs);

struct ApplicationNames {
  // This is the full path to the program (if it exists).
  fextl::string ProgramPath;
  // This is the program executable name (if it exists).
  fextl::string ProgramName;
};

struct PortableInformation {
  bool IsPortable;
  // Path of folder containing FEXInterpreter (including / at the end)
  fextl::string InterpreterPath;
};

/**
 * @param ExecFDInterp If FEX was executed with binfmt_misc FD argument
 * @param ProgramFDFromEnv The execveat FD argument passed through FEX
 *
 * @return The application name and path structure
 */
ApplicationNames GetApplicationNames(const fextl::vector<fextl::string>& Args, bool ExecFDInterp, int ProgramFDFromEnv);

/**
 * @brief Loads the FEX and application configurations for the application that is getting ready to run.
 *
 * @param ArgLoader Optional argument loader for argument based config options
 * @param ProgramName Optional program name, if non-empty application specific configurations will be loaded
 * @param envp Optional `envp` passed to main(...)
 */
void LoadConfig(fextl::unique_ptr<FEX::ArgLoader::ArgLoader> ArgLoader = {}, fextl::string ProgramName = {}, char** const envp = nullptr,
                const PortableInformation& PortableInfo = {});

const char* GetHomeDirectory();

fextl::string GetDataDirectory(const PortableInformation& PortableInfo);
fextl::string GetConfigDirectory(bool Global, const PortableInformation& PortableInfo);
fextl::string GetConfigFileLocation(bool Global, const PortableInformation& PortableInfo);

void InitializeConfigs(const PortableInformation& PortableInfo);

/**
 * @brief Loads the global FEX config
 *
 * @return unique_ptr for that layer
 */
fextl::unique_ptr<FEXCore::Config::Layer> CreateGlobalMainLayer();

/**
 * @brief Loads the main application config
 *
 * @param File Optional override to load a specific config file in to the main layer
 * Shouldn't be commonly used
 *
 * @return unique_ptr for that layer
 */
fextl::unique_ptr<FEXCore::Config::Layer> CreateMainLayer(const fextl::string* File = nullptr);
fextl::unique_ptr<FEXCore::Config::Layer> CreateUserOverrideLayer(std::string_view AppConfig);

/**
 * @brief Create an application configuration loader
 *
 * @param Filename Application filename component
 * @param Global Load the global configuration or user accessible file
 *
 * @return unique_ptr for that layer
 */
fextl::unique_ptr<FEXCore::Config::Layer> CreateAppLayer(const fextl::string& Filename, FEXCore::Config::LayerType Type);

/**
 * @brief iCreate an environment configuration loader
 *
 * @param _envp[] The environment array from main
 *
 * @return unique_ptr for that layer
 */
fextl::unique_ptr<FEXCore::Config::Layer> CreateEnvironmentLayer(char* const _envp[]);
} // namespace FEX::Config
