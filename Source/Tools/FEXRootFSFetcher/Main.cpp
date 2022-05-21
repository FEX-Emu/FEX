#include "XXFileHash.h"

#include "Common/ArgumentLoader.h"
#include "Common/Config.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

namespace Exec {
  int32_t ExecAndWaitForResponse(const char *path, char* const* args) {
    pid_t pid = fork();
    if (pid == 0) {
      execvp(path, args);
      _exit(-1);
    }
    else {
      int32_t Status{};
      waitpid(pid, &Status, 0);
      if (WIFEXITED(Status)) {
        return (int8_t)WEXITSTATUS(Status);
      }
    }

    return -1;
  }

  int32_t ExecAndWaitForResponseRedirect(const char *path, char* const* args, int stdoutRedirect = -2, int stderrRedirect = -2) {
    pid_t pid = fork();
    if (pid == 0) {
      if (stdoutRedirect == -1) {
        close(STDOUT_FILENO);
      }
      else if (stdoutRedirect == -2) {
        // Do nothing
      }
      else {
        if (stdoutRedirect != STDOUT_FILENO) {
          close(STDOUT_FILENO);
        }
        dup2(stdoutRedirect, STDOUT_FILENO);
      }
      if (stderrRedirect == -1) {
        close(STDERR_FILENO);
      }
      else if (stderrRedirect == -2) {
        // Do nothing
      }
      else {
        if (stderrRedirect != STDOUT_FILENO) {
          close(STDERR_FILENO);
        }
        dup2(stderrRedirect, STDERR_FILENO);
      }
      execvp(path, args);
      _exit(-1);
    }
    else {
      int32_t Status{};
      waitpid(pid, &Status, 0);
      if (WIFEXITED(Status)) {
        return (int8_t)WEXITSTATUS(Status);
      }
    }

    return -1;
  }

  std::string ExecAndWaitForResponseText(const char *path, char* const* args) {
    int fd[2];
    pipe(fd);

    pid_t pid = fork();

    if (pid == 0) {
      close(fd[0]); // Close read side

      // Redirect stdout to pipe
      dup2(fd[1], STDOUT_FILENO);

      // Close stderr
      close(STDERR_FILENO);

      // We can now close the pipe since the duplications take care of the rest
      close(fd[1]);

      execvp(path, args);
      _exit(-1);
    }
    else {
      close(fd[1]); // Close write side

      // Nothing larger than this
      char Buffer[1024]{};

      // Read the pipe until it closes
      while (read(fd[0], Buffer, sizeof(Buffer)));

      int32_t Status{};
      waitpid(pid, &Status, 0);
      if (WIFEXITED(Status)) {
        // Return what we've read
        close(fd[0]);
        return Buffer;
      }
    }

    return {};
  }
}

namespace WorkingAppsTester {
  static bool Has_Curl {false};
  static bool Has_Squashfuse {false};
  static bool Has_Unsquashfs {false};

  void CheckCurl() {
    // Check if curl exists on the host
    std::vector<const char*> ExecveArgs = {
      "curl",
      "-V",
      nullptr,
    };

    int32_t Result = Exec::ExecAndWaitForResponseRedirect(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data()), -1, -1);
    Has_Curl = Result != -1;
  }

  void CheckSquashfuse() {
    std::vector<const char*> ExecveArgs = {
      "squashfuse",
      "--help",
      nullptr,
    };

    int32_t Result = Exec::ExecAndWaitForResponseRedirect(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data()), -1, -1);
    Has_Squashfuse = Result != -1;
  }

  void CheckUnsquashfs() {
    std::vector<const char*> ExecveArgs = {
      "unsquashfs",
      "--help",
      nullptr,
    };

    int fd = ::syscall(SYS_memfd_create, "stdout", 0);
    int32_t Result = Exec::ExecAndWaitForResponseRedirect(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data()), fd, fd);
    Has_Unsquashfs = Result != -1;
    if (Has_Unsquashfs) {
      // Seek back to the start
      lseek(fd, 0, SEEK_SET);

      // Unsquashfs needs to support zstd
      // Scan its output to find the zstd compressor
      FILE *fp = fdopen(fd, "r");
      char *Line {nullptr};
      ssize_t NumRead;
      size_t Len;

      bool ReadingDecompressors = false;
      bool SupportsZSTD = false;
      while ((NumRead = getline(&Line, &Len, fp)) != -1) {
        if (!ReadingDecompressors) {
          if (strstr(Line, "Decompressors available")) {
            ReadingDecompressors = true;
          }
        }
        else {
          if (strstr(Line, "zstd")) {
            SupportsZSTD = true;
          }
        }
      }

      free(Line);
      fclose(fp);

      // Disable unsquashfs if it doesn't support ZSTD
      if (!SupportsZSTD) {
        Has_Unsquashfs = false;
      }
    }
    close(fd);
  }

  void Init() {
    CheckCurl();
    CheckSquashfuse();
    CheckUnsquashfs();
  }
}

namespace DistroQuery {
  struct DistroInfo {
    std::string DistroName;
    std::string DistroVersion;
    bool Unknown;
  };

  DistroInfo GetDistroInfo() {
    // Detect these files in order
    //
    // /etc/lsb-release
    // eg:
    // DISTRIB_ID=Ubuntu
    // DISTRIB_RELEASE=21.10
    // DISTRIB_CODENAME=impish
    // DISTRIB_DESCRIPTION="Ubuntu 21.10"
    //
    // /etc/os-release
    // eg:
    // PRETTY_NAME="Ubuntu 21.10"
    // NAME="Ubuntu"
    // VERSION_ID="21.10"
    // VERSION="21.10 (Impish Indri)"
    // VERSION_CODENAME=impish
    // ID=ubuntu
    // ID_LIKE=debian
    // HOME_URL="https://www.ubuntu.com/"
    // SUPPORT_URL="https://help.ubuntu.com/"
    // BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
    // PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
    // UBUNTU_CODENAME=impish
    //
    // /etc/debian_version
    // eg:
    // 11.0
    //
    // uname -r
    // eg:
    // 5.13.0-22-generic
    DistroInfo Info{};
    uint32_t FoundCount{};

    if (std::filesystem::exists("/etc/lsb-release")) {
      std::fstream File ("/etc/lsb-release", std::fstream::in);
      std::string Line;
      while (std::getline(File, Line)) {
        if (File.eof() || FoundCount == 2) {
          break;
        }

        std::stringstream ss(Line);
        std::string Key, Value;
        std::getline(ss, Key, '=');
        std::getline(ss, Value, '=');

        if (Key == "DISTRIB_ID") {
          auto ToLower = [](auto Str) {
            std::transform(Str.begin(), Str.end(), Str.begin(),
                    [](unsigned char c){ return std::tolower(c); });
            return Str;
          };
          Info.DistroName = ToLower(Value);
          ++FoundCount;
        }
        else if (Key == "DISTRIB_RELEASE") {
          Info.DistroVersion = Value;
          ++FoundCount;
        }
      }
    }

    if (FoundCount == 2) {
      Info.Unknown = false;
      return Info;
    }
    FoundCount = 0;

    if (std::filesystem::exists("/etc/os-release")) {
      std::fstream File ("/etc/os-release", std::fstream::in);
      std::string Line;
      while (std::getline(File, Line)) {
        if (File.eof() || FoundCount == 2) {
          break;
        }

        std::stringstream ss(Line);
        std::string Key, Value;
        std::getline(ss, Key, '=');
        std::getline(ss, Value, '=');

        if (Key == "ID") {
          Info.DistroName = Value;
          ++FoundCount;
        }
        else if (Key == "VERSION_ID") {
          // Strip the two quotes from the VERSION_ID
          Value = Value.substr(1, Value.size() - 2);
          Info.DistroVersion = Value;
          ++FoundCount;
        }
      }
    }

    if (FoundCount == 2) {
      Info.Unknown = false;
      return Info;
    }
    FoundCount = 0;

    if (std::filesystem::exists("/etc/debian_version")) {
      std::fstream File ("/etc/debian_version", std::fstream::in);
      std::string Line;

      Info.DistroName = "debian";
      ++FoundCount;
      while (std::getline(File, Line)) {
        Info.DistroVersion = Line;
        ++FoundCount;
      }
    }

    if (FoundCount == 2) {
      Info.Unknown = false;
      return Info;
    }

    Info.DistroName = "Unknown";
    Info.DistroVersion = {};
    Info.Unknown = true;
    return Info;
  }
}

namespace WebFileFetcher {
  struct FileTargets {
    // These two are for matching version checks
    std::string DistroMatch;
    std::string VersionMatch;

    // This is a human readable name
    std::string DistroName;

    // This is the URL
    std::string URL;

    // This is the hash of the file
    std::string Hash;
  };

  const static std::string DownloadURL = "https://rootfs.fex-emu.org/file/fex-rootfs/RootFS_links.txt";

  std::string DownloadToString(const std::string &URL) {
    std::string BigArgs =
    fmt::format("curl {}", URL);
    std::vector<const char*> ExecveArgs = {
      "/bin/sh",
      "-c",
      BigArgs.c_str(),
      nullptr,
    };

    return Exec::ExecAndWaitForResponseText(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data()));
  }

  bool DownloadToPath(const std::string &URL, const std::string &Path) {
    auto filename = URL.substr(URL.find_last_of('/') + 1);
    auto PathName = Path + filename;

    std::string BigArgs =
    fmt::format("curl {} -o {}", URL, PathName);
    std::vector<const char*> ExecveArgs = {
      "/bin/sh",
      "-c",
      BigArgs.c_str(),
      nullptr,
    };

    return Exec::ExecAndWaitForResponse(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data())) == 0;
  }

  bool DownloadToPathWithZenityProgress(const std::string &URL, const std::string &Path) {
    auto filename = URL.substr(URL.find_last_of('/') + 1);
    auto PathName = Path + filename;

    // -# for progress bar
    // -o for output file
    // -f for silent fail
    std::string CurlPipe = fmt::format("curl -C - -#f {} -o {} 2>&1", URL, PathName);
    const std::string StdBuf = "stdbuf -oL tr '\\r' '\\n'";
    const std::string SedBuf = "sed -u 's/[^0-9]*\\([0-9]*\\).*/\\1/'";
    // zenity --auto-close can't be used since `curl -C` for whatever reason prints 100% at the start.
    // Making zenity vanish immediately
    const std::string ZenityBuf = "zenity --time-remaining --progress --no-cancel --title 'Downloading'";
    std::string BigArgs =
    fmt::format("{} | {} | {} | {}", CurlPipe, StdBuf, SedBuf, ZenityBuf);
    std::vector<const char*> ExecveArgs = {
      "/bin/sh",
      "-c",
      BigArgs.c_str(),
      nullptr,
    };

    return Exec::ExecAndWaitForResponse(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data())) == 0;
  }

  std::vector<FileTargets> GetRootFSLinks() {
    // Decode the filetargets
    std::string Data = DownloadToString(DownloadURL);
    std::stringstream ss(Data);
    std::vector<FileTargets> Targets;

    while (!ss.eof()) {
      FileTargets Target;
      if (!std::getline(ss, Target.DistroMatch))
        break;
      if (!std::getline(ss, Target.VersionMatch))
        break;
      if (!std::getline(ss, Target.DistroName))
        break;
      if (!std::getline(ss, Target.URL))
        break;
      if (!std::getline(ss, Target.Hash))
        break;

      Targets.emplace_back(Target);
    }
    return Targets;
  }
}

namespace Zenity {
  bool ExecWithQuestion(const std::string &Question) {
    std::string TextArg = "--text=" + Question;
    const char *Args[] = {
      "zenity",
      "--question",
      TextArg.c_str(),
      nullptr,
    };

    int32_t Result = Exec::ExecAndWaitForResponse(Args[0], const_cast<char* const*>(Args));
    // 0 on Yes, 1 on No
    return Result == 0;
  }

  void ExecWithInfo(const std::string &Text) {
    std::string TextArg = "--text=" + Text;
    const char *Args[] = {
      "zenity",
      "--info",
      TextArg.c_str(),
      nullptr,
    };

    Exec::ExecAndWaitForResponse(Args[0], const_cast<char* const*>(Args));
  }

  bool AskForConfirmation(const std::string &Question) {
    return ExecWithQuestion(Question);
  }

  int32_t AskForConfirmationList(const std::string &Text, const std::vector<std::string> &Arguments) {
    std::string TextArg = "--text=" + Text;

    std::vector<const char*> ExecveArgs = {
      "zenity",
      "--list",
      TextArg.c_str(),
      "--hide-header",
      "--column=Index",
      "--column=Text",
      "--hide-column=1",
    };

    std::vector<std::string> NumberArgs;
    for (size_t i = 0; i < Arguments.size(); ++i) {
      NumberArgs.emplace_back(std::to_string(i));
    }

    for (size_t i = 0; i < Arguments.size(); ++i) {
      const auto &Arg = Arguments[i];
      ExecveArgs.emplace_back(NumberArgs[i].c_str());
      ExecveArgs.emplace_back(Arg.c_str());
    }
    ExecveArgs.emplace_back(nullptr);

    auto Result = Exec::ExecAndWaitForResponseText(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data()));
    if (Result.empty()) {
      return -1;
    }
    return std::stoi(Result);
  }

  int32_t AskForComplexConfirmationList(const std::string &Text, const std::vector<std::string> &Arguments) {
    std::string TextArg = "--text=" + Text;

    std::vector<const char*> ExecveArgs = {
      "zenity",
      "--list",
      TextArg.c_str(),
    };

    for (auto &Arg : Arguments) {
      ExecveArgs.emplace_back(Arg.c_str());
    }
    ExecveArgs.emplace_back(nullptr);

    auto Result = Exec::ExecAndWaitForResponseText(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data()));
    if (Result.empty()) {
      return -1;
    }
    return std::stoi(Result);
  }

  int32_t AskForDistroSelection(DistroQuery::DistroInfo &Info, std::vector<WebFileFetcher::FileTargets> &Targets) {
    // Search for an exact match
    int32_t DistroIndex = -1;
    if (!Info.Unknown) {
      for (size_t i = 0; i < Targets.size(); ++i) {
        const auto &Target = Targets[i];

        bool ExactMatch = Target.DistroMatch == Info.DistroName &&
            Target.VersionMatch == Info.DistroVersion;
        if (ExactMatch) {
          std::string Question = fmt::format("Found exact match for distro '{}'. Do you want to select this image?", Target.DistroName);
          if (ExecWithQuestion(Question)) {
            DistroIndex = i;
            break;
          }
        }
      }
    }

    if (DistroIndex != -1) {
      return DistroIndex;
    }

    std::vector<std::string> Args;

    Args.emplace_back("--column=Index");
    Args.emplace_back("--column=Distro");
    Args.emplace_back("--hide-column=1");
    for (size_t i = 0; i < Targets.size(); ++i) {
      const auto &Target = Targets[i];
      Args.emplace_back(std::to_string(i));
      Args.emplace_back(Target.DistroName);
    }

    std::string Text = "RootFS list selection";
    return AskForComplexConfirmationList(Text, Args);
  }

  bool ValidateCheckExists(const WebFileFetcher::FileTargets &Target) {
    std::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
    auto filename = Target.URL.substr(Target.URL.find_last_of('/') + 1);
    auto PathName = RootFS + filename;
    uint64_t ExpectedHash = std::stoul(Target.Hash, nullptr, 16);

    std::error_code ec;
    if (std::filesystem::exists(PathName, ec)) {
      const std::vector<std::string> Args {
        "Overwrite",
        "Validate",
      };
      std::string Text = filename + " already exists. What do you want to do?";
      int Result = AskForConfirmationList(Text, Args);
      if (Result == -1) {
        return false;
      }

      auto Res = XXFileHash::HashFile(PathName);
      if (Result == 0) {
        if (Res.first == true &&
            Res.second == ExpectedHash) {
          std::string Text = fmt::format("{} matches expected hash. Skipping download", filename);
          ExecWithInfo(Text);
          return false;
        }
      }
      else if (Result == 1) {
        if (Res.first == false ||
            Res.second != ExpectedHash) {
          return AskForConfirmation("RootFS doesn't match hash!\nDo you want to redownload?");
        }
        else {
          std::string Text = fmt::format("{} matches expected hash", filename);
          ExecWithInfo(Text);
          return false;
        }
      }
    }

    return true;
  }

  bool ValidateDownloadSelection(const WebFileFetcher::FileTargets &Target) {
    std::string Text = fmt::format("Selected Rootfs: {}\n", Target.DistroName);
    Text += fmt::format("\tURL: {}\n", Target.URL);
    Text += fmt::format("Are you sure that you want to download this image");

    if (AskForConfirmation(Text)) {
      std::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
      std::error_code ec{};
      if (!std::filesystem::exists(RootFS, ec)) {
        // Doesn't exist, create the the folder as a user convenience
        if (!std::filesystem::create_directories(RootFS, ec)) {
          // Well I guess we failed
          Text = fmt::format("Couldn't create {} path for storing RootFS", RootFS);
          ExecWithInfo(Text);
          return false;
        }
      }

      if (!WebFileFetcher::DownloadToPathWithZenityProgress(Target.URL, RootFS)) {
        return false;
      }

      return true;
    }
    return false;
  }
}

namespace TTY {
  bool AskForConfirmation(const std::string &Question) {
    auto ToLowerInPlace = [](auto &Str) {
      std::transform(Str.begin(), Str.end(), Str.begin(),
              [](unsigned char c){ return std::tolower(c); });
    };

    std::cout << Question << std::endl;
    std::cout << "Response {y,yes,1} or {n,no,0}" << std::endl;
    std::string Response;
    std::cin >> Response;

    ToLowerInPlace(Response);
    if (Response == "y" ||
        Response == "yes" ||
        Response == "1") {
      return true;
    }
    else if (Response == "n" ||
      Response == "no" ||
      Response == "0") {
      return false;
    }
    else {
      std::cout << "Unknown response. Assuming no" << std::endl;
      return false;
    }
  }

  void ExecWithInfo(const std::string &Text) {
    std::cout << Text << std::endl;
  }

  int32_t AskForConfirmationList(const std::string &Text, const std::vector<std::string> &List) {
    fmt::print("{}\n", Text);
    fmt::print("Options:\n");
    fmt::print("\t0: Cancel\n");

    for (size_t i = 0; i < List.size(); ++i) {
      fmt::print("\t{}: {}\n", i+1, List[i]);
    }

    fmt::print("\t\nResponse {{1-{}}} or 0 to cancel\n", List.size());
    std::string Response;
    std::cin >> Response;

    int32_t ResponseInt = std::stoi(Response);
    if (ResponseInt == 0) {
      return -1;
    }
    else if (ResponseInt >= 1 &&
      (ResponseInt - 1) < List.size()) {
      return ResponseInt - 1;
    }
    else {
      std::cout << "Unknown response. Assuming cancel" << std::endl;
      return -1;
    }
  }

  int32_t AskForDistroSelection(DistroQuery::DistroInfo &Info, std::vector<WebFileFetcher::FileTargets> &Targets) {
    // Search for an exact match
    int32_t DistroIndex = -1;
    if (!Info.Unknown) {
      for (size_t i = 0; i < Targets.size(); ++i) {
        const auto &Target = Targets[i];

        bool ExactMatch = Target.DistroMatch == Info.DistroName &&
            Target.VersionMatch == Info.DistroVersion;
        if (ExactMatch) {
          std::string Question = fmt::format("Found exact match for distro '{}'. Do you want to select this image?", Target.DistroName);
          if (AskForConfirmation(Question)) {
            DistroIndex = i;
            break;
          }
        }
      }
    }

    if (DistroIndex != -1) {
      return DistroIndex;
    }

    std::vector<std::string> Args;
    for (size_t i = 0; i < Targets.size(); ++i) {
      const auto &Target = Targets[i];
      Args.emplace_back(Target.DistroName);
    }

    std::string Text = "RootFS list selection";
    return AskForConfirmationList(Text, Args);
  }

  bool ValidateCheckExists(const WebFileFetcher::FileTargets &Target) {
    std::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
    auto filename = Target.URL.substr(Target.URL.find_last_of('/') + 1);
    auto PathName = RootFS + filename;
    uint64_t ExpectedHash = std::stoul(Target.Hash, nullptr, 16);

    std::error_code ec;
    if (std::filesystem::exists(PathName, ec)) {
      const std::vector<std::string> Args {
        "Overwrite",
        "Validate",
      };
      std::string Text = filename + " already exists. What do you want to do?";
      int Result = AskForConfirmationList(Text, Args);
      if (Result == -1) {
        return false;
      }
      fmt::print("Validating RootFS hash...\n");
      auto Res = XXFileHash::HashFile(PathName);
      if (Result == 0) {
        if (Res.first == true &&
            Res.second == ExpectedHash) {
          fmt::print("{} matches expected hash. Skipping downloading\n", filename);
          return false;
        }
      }
      else if (Result == 1) {
        if (Res.first == false ||
            Res.second != ExpectedHash) {
          fmt::print("RootFS doesn't match hash!\n");
          return AskForConfirmation("Do you want to redownload?");
        }
        else {
          fmt::print("{} matches expected hash\n", filename);
          return false;
        }
      }
    }

    return true;
  }

  bool ValidateDownloadSelection(const WebFileFetcher::FileTargets &Target) {
    fmt::print("Selected Rootfs: {}\n", Target.DistroName);
    fmt::print("\tURL: {}\n", Target.URL);

    if (AskForConfirmation("Are you sure that you want to download this image")) {
      std::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
      std::error_code ec{};
      if (!std::filesystem::exists(RootFS, ec)) {
        // Doesn't exist, create the the folder as a user convenience
        if (!std::filesystem::create_directories(RootFS, ec)) {
          // Well I guess we failed
          fmt::print("Couldn't create {} path for storing RootFS\n", RootFS);
          return false;
        }
      }
      auto DoDownload = [&Target, &RootFS]() -> bool {
        if (!WebFileFetcher::DownloadToPath(Target.URL, RootFS)) {
          fmt::print("Couldn't download RootFS\n");
          return false;
        }

        return true;
      };

      while (DoDownload() == false) {
        if (AskForConfirmation("Curl RootFS download failed. Do you want to retry?")) {
          // Loop to retry
        }
        else {
          return false;
        }
      }

      // Got here then we passed
      return true;
    }
    return false;
  }
}

namespace {
  bool IsTTY{};

  std::function<bool(const std::string &Question)> _AskForConfirmation;
  std::function<void(const std::string &Text)> _ExecWithInfo;
  std::function<int32_t(const std::string &Text, const std::vector<std::string> &List)> _AskForConfirmationList;
  std::function<int32_t(DistroQuery::DistroInfo &Info, std::vector<WebFileFetcher::FileTargets> &Targets)> _AskForDistroSelection;
  std::function<bool(const WebFileFetcher::FileTargets &Target)> _ValidateCheckExists;
  std::function<bool(const WebFileFetcher::FileTargets &Target)> _ValidateDownloadSelection;

  void CheckTTY() {
    IsTTY = isatty(STDOUT_FILENO);

    if (IsTTY) {
      _AskForConfirmation = TTY::AskForConfirmation;
      _ExecWithInfo = TTY::ExecWithInfo;
      _AskForConfirmationList = TTY::AskForConfirmationList;
      _AskForDistroSelection = TTY::AskForDistroSelection;
      _ValidateCheckExists = TTY::ValidateCheckExists;
      _ValidateDownloadSelection = TTY::ValidateDownloadSelection;
    }
    else {
      _AskForConfirmation = Zenity::AskForConfirmation;
      _ExecWithInfo = Zenity::ExecWithInfo;
      _AskForConfirmationList = Zenity::AskForConfirmationList;
      _AskForDistroSelection = Zenity::AskForDistroSelection;
      _ValidateCheckExists = Zenity::ValidateCheckExists;
      _ValidateDownloadSelection = Zenity::ValidateDownloadSelection;
    }
  }

  bool AskForConfirmation(const std::string &Question) {
    return _AskForConfirmation(Question);
  }

  void ExecWithInfo(const std::string &Text) {
    _ExecWithInfo(Text);
  }

  int32_t AskForConfirmationList(const std::string &Text, const std::vector<std::string> &Arguments) {
    return _AskForConfirmationList(Text, Arguments);
  }

  int32_t AskForDistroSelection(std::vector<WebFileFetcher::FileTargets> &Targets) {
    auto Info = DistroQuery::GetDistroInfo();
    return _AskForDistroSelection(Info, Targets);
  }

  bool ValidateCheckExists(const WebFileFetcher::FileTargets &Target) {
    return _ValidateCheckExists(Target);
  }

  bool ValidateDownloadSelection(const WebFileFetcher::FileTargets &Target) {
    return _ValidateDownloadSelection(Target);
  }
}

namespace ConfigSetter {
  void SetRootFSAsDefault(const std::string &RootFS) {
    std::string Filename = FEXCore::Config::GetConfigFileLocation();
    auto LoadedConfig = FEXCore::Config::CreateMainLayer(&Filename);
    LoadedConfig->Load();
    LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_ROOTFS, RootFS);
    FEX::Config::SaveLayerToJSON(Filename, LoadedConfig.get());
  }
}

namespace UnSquash {
  bool UnsquashRootFS(const std::string &Path, const std::string &RootFS, const std::string &FolderName) {
    auto TargetFolder = Path + FolderName;

    bool Extract = true;
    std::error_code ec;
    if (std::filesystem::exists(TargetFolder, ec)) {
      std::string Question = FolderName + " Already exists. Overwrite?";
      if (AskForConfirmation(Question)) {
        if (std::filesystem::remove_all(TargetFolder, ec) != ~0ULL) {
          Extract = true;
        }
      }
    }

    if (Extract) {
      const std::vector<const char*> ExecveArgs = {
        "unsquashfs",
        "-f",
        "-d",
        TargetFolder.c_str(),
        RootFS.c_str(),
        nullptr,
      };

      return Exec::ExecAndWaitForResponse(ExecveArgs[0], const_cast<char* const*>(ExecveArgs.data())) == 0;
    }

    return false;
  }
}

int main(int argc, char **argv, char **const envp) {
  CheckTTY();
  FEX::Config::LoadConfig(
    true,
    false,
    argc, argv, envp
  );

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  if (Args.size()) {
    auto Res = XXFileHash::HashFile(Args[0]);
    if (Res.first) {
      fmt::print("{} has hash: {:x}\n", Args[0], Res.second);
    }
    else {
      fmt::print("Couldn't generate hash for {}\n", Args[0]);
    }
    return 0;
  }

  WorkingAppsTester::Init();
  // Check if curl exists on the host
  if (!WorkingAppsTester::Has_Curl) {
    ExecWithInfo("curl is required to use this tool. Please install curl before using.");
    return -1;
  }

  FEX_CONFIG_OPT(LDPath, ROOTFS);

  std::error_code ec;
  std::string Question{};
  if (LDPath().empty() ||
      std::filesystem::exists(LDPath(), ec) == false) {
    Question = "RootFS not found. Do you want to try and download one?";
  }
  else {
    Question = "RootFS is already in use. Do you want to check the download list?";
  }

  if (AskForConfirmation(Question)) {
    auto Targets = WebFileFetcher::GetRootFSLinks();
    int32_t DistroIndex = AskForDistroSelection(Targets);
    if (DistroIndex != -1) {
      const auto &Target = Targets[DistroIndex];
      std::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
      auto filename = Target.URL.substr(Target.URL.find_last_of('/') + 1);
      auto PathName = RootFS + filename;

      if (!ValidateCheckExists(Target)) {
        // Keep going
      }
      else {
        auto ValidateDownload = [&Target, &PathName]() -> std::pair<int32_t, bool> {
          std::error_code ec;
          if (ValidateDownloadSelection(Target)) {
            uint64_t ExpectedHash = std::stoul(Target.Hash, nullptr, 16);

            if (std::filesystem::exists(PathName, ec)) {
              auto Res = XXFileHash::HashFile(PathName);
              if (Res.first == false ||
                  Res.second != ExpectedHash) {
                std::string Text = fmt::format("Couldn't hash the rootfs or hash didn't match\n");
                Text += fmt::format("Hash {:x} != Expected Hash {:x}\n", Res.second, ExpectedHash);
                ExecWithInfo(Text);
                return std::make_pair(-1, true);
              }
            }
            else {
              ExecWithInfo("Correctly downloaded RootFS but doesn't exist?");
              return std::make_pair(-1, false);
            }
          }
          else {
            ExecWithInfo("Couldn't download rootfs for some reason.");
            return std::make_pair(-1, false);
          }

          return std::make_pair(0, false);
        };

        std::pair<int32_t, bool> Result{};
        while ((Result = ValidateDownload()).second == true &&
            Result.first == -1) {

          if (AskForConfirmation("Do you want to try downloading the RootFS again?")) {
            // Continue the loop
          }
          else {
            // Didn't want to retry, just exit now
            return Result.first;
          }
        }

        // Early exit on other errors
        if (Result.first == -1 &&
            Result.second == false) {
          return Result.first;
        }
      }

      std::vector<std::string> Args = {
        "Extract",
        "As-Is",
      };

      int32_t Result{};
      if (WorkingAppsTester::Has_Unsquashfs) {
        if (WorkingAppsTester::Has_Squashfuse) {
          Result = AskForConfirmationList("Do you wish to extract the squashfs file or use it as-is?", Args);
        }
        else {
          Args.pop_back();
          Result = AskForConfirmationList("Squashfuse doesn't work. Do you wish to extract the squashfs file?", Args);
        }
      }
      else {
        if (WorkingAppsTester::Has_Squashfuse) {
          Args.erase(Args.begin());
          Result = AskForConfirmationList("Unsquashfs doesn't work. Do you want to use the squashfs file as-is?", Args);
          if (Result == 0) {
            // We removed an argument, Just change "As-Is" from 0 to 1 for later logic to work
            Result = 1;
          }
        }
        else {
          Args.erase(Args.begin());
          ExecWithInfo("Unsquashfs and squashfuse isn't working. Leaving rootfs as-is");
          Result = -1;
        }
      }
      if (Result == -1 ||
          Result == 1) {
        // Nothing
        // As-Is
      }
      else if (Result == 0) {
        auto FolderName = filename.substr(0, filename.find_last_of('.'));
        if (UnSquash::UnsquashRootFS(RootFS, PathName, FolderName)) {
          // Remove the .sqsh suffix since we extracted to that
          filename = FolderName;
        }
      }

      if (AskForConfirmation("Do you wish to set this RootFS as default?")) {
        ConfigSetter::SetRootFSAsDefault(filename);
        auto Text = fmt::format("{} set as default RootFS\n", filename);
        ExecWithInfo(Text);
      }
    }
  }

  return 0;
}
