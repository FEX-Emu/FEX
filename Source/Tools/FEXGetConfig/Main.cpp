#include "ConfigDefines.h"
#include "OptionParser.h"
#include "Common/RootFSSetup.h"
#include "git_version.h"
#include <FEXCore/Config/Config.h>

#include <filesystem>
#include <stdio.h>
#include <string>

int main(int argc, char **argv, char **envp) {
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEXCore::Config::CreateMainLayer());
  // No FEX arguments passed through command line
  FEXCore::Config::AddLayer(FEXCore::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

  // Load the arguments
  optparse::OptionParser Parser = optparse::OptionParser()
    .description("Simple application to get a couple of FEX options");

  Parser.add_option("--install-prefix")
    .action("store_true")
    .help("Print the FEX install prefix");

  Parser.add_option("--app")
    .help("Load an application profile for this application if it exists");

  Parser.add_option("--current-rootfs")
    .action("store_true")
    .help("Print the directory that contains the FEX rootfs. Mounted in the case of squashfs");

  Parser.add_option("--current-rootfs-lock")
    .action("store_true")
    .help("SquashFS lock file if squashfs is mounted");

  Parser.add_option("--current-rootfs-socket")
    .action("store_true")
    .help("SquashFS socket file if squashfs is mounted");

  Parser.add_option("--version")
    .action("store_true")
    .help("Print the installed FEX-Emu version");

  optparse::Values Options = Parser.parse_args(argc, argv);

  if (Options.is_set_by_user("app")) {
    // Load the application config if one was provided
    auto ProgramName = std::filesystem::path(Options["app"]).filename();
    FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(ProgramName, true));
    FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(ProgramName, false));
  }

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();

  if (Options.is_set_by_user("version")) {
    fprintf(stdout, GIT_DESCRIBE_STRING "\n");
  }

  if (Options.is_set_by_user("install_prefix")) {
    fprintf(stdout, FEX_INSTALL_PREFIX "\n");
  }

  if (Options.is_set_by_user("current_rootfs_lock")) {
    auto LockFile = FEX::RootFS::GetRootFSLockFile();
    if (FEX::RootFS::CheckLockExists(LockFile)) {
      fprintf(stdout, "%s\n", LockFile.c_str());
    }
  }

  if (Options.is_set_by_user("current_rootfs_socket")) {
    auto LockFile = FEX::RootFS::GetRootFSLockFile();
    std::string MountPath;
    if (FEX::RootFS::CheckLockExists(LockFile, &MountPath)) {
      std::string SocketPath = FEX::RootFS::GetRootFSSocketFile(MountPath);
      fprintf(stdout, "%s\n", SocketPath.c_str());
    }
  }

  if (Options.is_set_by_user("current_rootfs")) {
    auto LockFile = FEX::RootFS::GetRootFSLockFile();
    std::string MountPath;
    if (FEX::RootFS::CheckLockExists(LockFile, &MountPath)) {
      fprintf(stdout, "%s\n", MountPath.c_str());
    }
  }

  return 0;
}
