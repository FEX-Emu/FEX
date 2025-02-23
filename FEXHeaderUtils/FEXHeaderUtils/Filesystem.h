// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/memory_resource.h>
#include <FEXCore/fextl/string.h>

#include <algorithm>
#include <fcntl.h>
#include <memory_resource>
#include <string>
#ifndef _WIN32
#include <linux/limits.h>
#include <sys/sendfile.h>
#else
#include <filesystem>
#endif
#include <sys/stat.h>
#include <unistd.h>

namespace FHU::Filesystem {
enum class CreateDirectoryResult {
  CREATED,
  EXISTS,
  ERROR,
};

enum class CopyOptions {
  NONE,
  SKIP_EXISTING,
  OVERWRITE_EXISTING,
};

/**
 * @brief Check if a filepath exists.
 *
 * @param Path The path to check for.
 *
 * @return True if the file exists, False if it doesn't.
 */
inline bool Exists(const char* Path) {
  return access(Path, F_OK) == 0;
}

inline bool Exists(const fextl::string& Path) {
  return access(Path.c_str(), F_OK) == 0;
}

/**
 * @brief Renames a file and overwrites if it already exists.
 *
 * @return No error on rename.
 */
[[nodiscard]]
inline std::error_code RenameFile(const fextl::string& From, const fextl::string& To) {
  return rename(From.c_str(), To.c_str()) == 0 ? std::error_code {} : std::make_error_code(std::errc::io_error);
}

#ifndef _WIN32
inline bool ExistsAt(int FD, const fextl::string& Path) {
  return faccessat(FD, Path.c_str(), F_OK, 0) == 0;
}

/**
 * @brief Creates a directory at the provided path.
 *
 * @param Path The path to create a directory at.
 *
 * @return Result enum depending.
 */
inline CreateDirectoryResult CreateDirectory(const fextl::string& Path) {
  auto Result = ::mkdir(Path.c_str(), 0777);
  if (Result == 0) {
    return CreateDirectoryResult::CREATED;
  }

  if (Result == -1 && errno == EEXIST) {
    // If it exists, we need to check if it is a file or folder.
    struct stat buf;
    if (stat(Path.c_str(), &buf) == 0) {
      // Check to see if the path is a file or folder. Following symlinks.
      return S_ISDIR(buf.st_mode) ? CreateDirectoryResult::EXISTS : CreateDirectoryResult::ERROR;
    }
  }

  // Couldn't create, or the path that existed wasn't a folder.
  return CreateDirectoryResult::ERROR;
}

/**
 * @brief Creates a directory tree with the provided path.
 *
 * @param Path The path to create a tree at.
 *
 * @return True if the directory tree was created or already exists.
 */
inline bool CreateDirectories(const fextl::string& Path) {
  // Try to create the directory initially.
  if (CreateDirectory(Path) != CreateDirectoryResult::ERROR) {
    return true;
  }

  // Walk the path in reverse and create paths as we go.
  fextl::string TmpPath {Path.substr(0, Path.rfind('/', Path.size() - 1))};
  if (!TmpPath.empty() && CreateDirectories(TmpPath)) {
    return CreateDirectory(Path) != CreateDirectoryResult::ERROR;
  }
  return false;
}

/**
 * @brief Extracts the filename component from a file path.
 *
 * @param Path The path to create a directory at.
 *
 * @return The filename component of the path.
 */
inline fextl::string GetFilename(const fextl::string& Path) {
  auto LastSeparator = Path.rfind('/');
  if (LastSeparator == fextl::string::npos) {
    // No separator. Likely relative `.`, `..`, `<Application Name>`, or empty string.
    return Path;
  }

  return Path.substr(LastSeparator + 1);
}

inline std::string_view GetFilename(std::string_view Path) {
  auto LastSeparator = Path.rfind('/');
  if (LastSeparator == fextl::string::npos) {
    // No separator. Likely relative `.`, `..`, `<Application Name>`, or empty string.
    return Path;
  }

  return Path.substr(LastSeparator + 1);
}

inline fextl::string ParentPath(const fextl::string& Path) {
  auto LastSeparator = Path.rfind('/');

  if (LastSeparator == fextl::string::npos) {
    // No separator. Likely relative `.`, `..`, `<Application Name>`, or empty string.
    if (Path == "." || Path == "..") {
      // In this edge-case, return nothing to match std::filesystem::path::parent_path behaviour.
      return {};
    }
    return Path;
  }

  if (LastSeparator == 0) {
    // In the case of root, just return.
    return "/";
  }

  auto SubString = Path.substr(0, LastSeparator);

  while (SubString.size() > 1 && SubString.ends_with("/")) {
    // If the substring still ended with `/` then we need to string that off as well.
    --LastSeparator;
    SubString = Path.substr(0, LastSeparator);
  }

  return SubString;
}

inline bool IsRelative(const std::string_view Path) {
  return !Path.starts_with('/');
}

inline bool IsAbsolute(const std::string_view Path) {
  return Path.starts_with('/');
}

/**
 * @brief Copy a file from a location to another
 *
 * Behaves similarly to std::filesystem::copy_file but with less copy options.
 *
 * @param From Source file location.
 * @param To Destination file location.
 * @param Options Copy options.
 *
 * @return True if the copy succeeded, false otherwise.
 */
inline bool CopyFile(const fextl::string& From, const fextl::string& To, CopyOptions Options = CopyOptions::NONE) {
  const bool DestExists = Exists(To);
  if (Options == CopyOptions::SKIP_EXISTING && DestExists) {
    // If the destination file exists already and the skip existing flag is set then
    // return true without error.
    return true;
  }

  if (Options == CopyOptions::OVERWRITE_EXISTING && DestExists) {
    // If we are overwriting and the file exists then we want to use `sendfile` to overwrite
    int SourceFD = open(From.c_str(), O_RDONLY | O_CLOEXEC);
    if (SourceFD == -1) {
      return false;
    }

    int DestinationFD = open(To.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0200);
    if (DestinationFD == -1) {
      close(SourceFD);
      return false;
    }

    struct stat buf;
    if (fstat(SourceFD, &buf) != 0) {
      close(DestinationFD);
      close(SourceFD);
      return false;
    }

    // Set the destination permissions to the original source permissions.
    if (fchmod(DestinationFD, buf.st_mode) != 0) {
      close(DestinationFD);
      close(SourceFD);
      return false;
    }
    bool Result = sendfile(DestinationFD, SourceFD, nullptr, buf.st_size) == buf.st_size;
    close(DestinationFD);
    close(SourceFD);
    return Result;
  }

  if (!DestExists) {
    // If the destination doesn't exist then just use rename.
    return rename(From.c_str(), To.c_str()) == 0;
  }

  return false;
}

inline fextl::string LexicallyNormal(const fextl::string& Path) {
  const auto PathSize = Path.size();

  // Early exit on empty paths.
  if (PathSize == 0) {
    return {};
  }

  const auto IsAbsolutePath = IsAbsolute(Path);
  const auto EndsWithSeparator = Path.ends_with('/');
  // Count the number of separators up front
  const auto SeparatorCount = std::count(Path.begin(), Path.end(), '/');

  // Use std::list to store path elements to avoid iterator invalidation on insert/erase.
  // The list is allocated on stack to be more optimal. The size is determined by the
  // maximum number of list objects (separator count plus 2) multiplied by the list
  // element size (32-bytes per element: the string_view itself and the prev/next pointers).
  size_t DataSize = (sizeof(std::string_view) + sizeof(void*) * 2) * (SeparatorCount + 2);
  void* Data = alloca(DataSize);
  fextl::pmr::fixed_size_monotonic_buffer_resource mbr(Data, DataSize);
  std::pmr::polymorphic_allocator<std::byte> pa {&mbr};
  std::pmr::list<std::string_view> Parts {pa};

  size_t CurrentOffset {};
  do {
    auto FoundSeperator = Path.find('/', CurrentOffset);
    if (FoundSeperator == Path.npos) {
      FoundSeperator = PathSize;
    }

    const auto Begin = Path.begin() + CurrentOffset;
    const auto End = Path.begin() + FoundSeperator;
    const auto Size = End - Begin;

    // Only insert parts that contain data.
    if (Size != 0) {
      Parts.emplace_back(std::string_view(Begin, End));
    }

    if (Size == 0) {
      // If the view is empty, skip over the separator.
      FoundSeperator += 1;
    }

    CurrentOffset = FoundSeperator;
  } while (CurrentOffset != PathSize);

  size_t CurrentIterDistance {};
  for (auto iter = Parts.begin(); iter != Parts.end();) {
    auto& Part = *iter;
    if (Part == ".") {
      // Erase '.' directory parts if not at root.
      if (CurrentIterDistance > 0 || IsAbsolutePath) {
        // Erasing this iterator, don't increase iter distances
        iter = Parts.erase(iter);
        continue;
      }
    }

    if (Part == "..") {
      if (CurrentIterDistance > 0) {
        // If not at root then remove both this iterator and the previous one.
        // ONLY if the previous iterator is also not ".."
        //
        // If the previous iterator is '.' then /only/ erase the previous iterator.
        auto PreviousIter = iter;
        --PreviousIter;

        if (*PreviousIter == ".") {
          // Erasing the previous iterator, iterator distance has subtracted by one
          --CurrentIterDistance;
          Parts.erase(PreviousIter);
        } else if (*PreviousIter != "..") {
          // Erasing the previous iterator, iterator distance has subtracted by one
          // Also erasing current iterator, which means iterator distance also doesn't increase by one.
          --CurrentIterDistance;
          Parts.erase(PreviousIter);
          iter = Parts.erase(iter);
          continue;
        }
      } else if (IsAbsolutePath) {
        // `..` at the base. Just remove this
        iter = Parts.erase(iter);
        continue;
      }
    }

    // Interator distance increased by one.
    ++CurrentIterDistance;
    ++iter;
  }


  // Add a final separator unless the last element is ellipses.
  const bool NeedsFinalSeparator = EndsWithSeparator && (!Parts.empty() && Parts.back() != "." && Parts.back() != "..");
  return fextl::fmt::format("{}{}{}", IsAbsolutePath ? "/" : "", fmt::join(Parts, "/"), NeedsFinalSeparator ? "/" : "");
}

inline char* Absolute(const char* Path, char Fill[PATH_MAX]) {
  return realpath(Path, Fill);
}
#else
inline fextl::string PathToString(const std::filesystem::path& path) {
  return path.string<char, std::char_traits<char>, fextl::FEXAlloc<char>>();
}

inline CreateDirectoryResult CreateDirectory(const fextl::string& Path) {
  std::error_code ec;
  if (std::filesystem::exists(Path, ec)) {
    return CreateDirectoryResult::EXISTS;
  }

  return std::filesystem::create_directory(Path, ec) ? CreateDirectoryResult::CREATED : CreateDirectoryResult::ERROR;
}

inline bool CreateDirectories(const fextl::string& Path) {
  std::error_code ec;
  return std::filesystem::exists(Path, ec) || std::filesystem::create_directories(Path, ec);
}

inline fextl::string GetFilename(const fextl::string& Path) {
  return PathToString(std::filesystem::path(Path).filename());
}

inline fextl::string ParentPath(const fextl::string& Path) {
  return PathToString(std::filesystem::path(Path).parent_path());
}

inline bool IsRelative(const std::string_view Path) {
  return std::filesystem::path(Path).is_relative();
}

inline bool IsAbsolute(const std::string_view Path) {
  return std::filesystem::path(Path).is_absolute();
}

inline bool CopyFile(const fextl::string& From, const fextl::string& To, CopyOptions Options = CopyOptions::NONE) {
  std::filesystem::copy_options options {};
  if (Options == CopyOptions::SKIP_EXISTING) {
    options = std::filesystem::copy_options::skip_existing;
  } else if (Options == CopyOptions::OVERWRITE_EXISTING) {
    options = std::filesystem::copy_options::overwrite_existing;
  }

  std::error_code ec;
  return std::filesystem::copy_file(From, To, options, ec);
}

inline fextl::string LexicallyNormal(const fextl::string& Path) {
  return PathToString(std::filesystem::path(Path).lexically_normal());
}

inline char* Absolute(const char* Path, char Fill[PATH_MAX]) {
  std::error_code ec;
  const auto PathAbsolute = std::filesystem::absolute(Path, ec);
  if (!ec) {
    strncpy(Fill, PathAbsolute.string().c_str(), sizeof(*Fill));
    return Fill;
  }

  return nullptr;
}
#endif

} // namespace FHU::Filesystem
