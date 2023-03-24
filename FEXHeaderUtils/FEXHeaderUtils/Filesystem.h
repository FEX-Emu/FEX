#pragma once
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/memory_resource.h>
#include <FEXCore/fextl/string.h>

#include <algorithm>
#include <fcntl.h>
#include <memory_resource>
#include <string>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

namespace FHU::Filesystem {
  /**
   * @brief Check if a filepath exists.
   *
   * @param Path The path to check for.
   *
   * @return True if the file exists, False if it doesn't.
   */
  inline bool Exists(const char *Path) {
    return access(Path, F_OK) == 0;
  }

  inline bool Exists(const fextl::string &Path) {
    return access(Path.c_str(), F_OK) == 0;
  }

  /**
   * @brief Creates a directory at the provided path.
   *
   * @param Path The path to create a directory at.
   *
   * @return True if the directory was created or already exists.
   */
  inline bool CreateDirectory(const fextl::string &Path) {
    auto Result = ::mkdir(Path.c_str(), 0777);
    if (Result == 0) {
      return true;
    }

    if (Result == -1 && errno == EEXIST) {
      // If it exists, we need to check if it is a file or folder.
      struct stat buf;
      if (stat(Path.c_str(), &buf) == 0) {
        // Check to see if the path is a file or folder. Following symlinks.
        return S_ISDIR(buf.st_mode);
      }
    }

    // Couldn't create, or the path that existed wasn't a folder.
    return false;
  }

  /**
   * @brief Creates a directory tree with the provided path.
   *
   * @param Path The path to create a tree at.
   *
   * @return True if the directory tree was created or already exists.
   */
  inline bool CreateDirectories(const fextl::string &Path) {
    // Try to create the directory initially.
    if (CreateDirectory(Path)) {
      return true;
    }

    // Walk the path in reverse and create paths as we go.
    fextl::string TmpPath {Path.substr(0, Path.rfind('/', Path.size() - 1))};
    if (!TmpPath.empty() && CreateDirectories(TmpPath)) {
      return CreateDirectory(Path);
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
  inline fextl::string GetFilename(const fextl::string &Path) {
    auto LastSeparator = Path.rfind('/');
    if (LastSeparator == fextl::string::npos) {
      // No separator. Likely relative `.`, `..`, `<Application Name>`, or empty string.
      return Path;
    }

    return Path.substr(LastSeparator + 1);
  }

  inline fextl::string ParentPath(const fextl::string &Path) {
    auto LastSeparator = Path.rfind('/');
    if (LastSeparator == fextl::string::npos) {
      // No separator. Likely relative `.`, `..`, `<Application Name>`, or empty string.
      return Path;
    }

    return Path.substr(0, LastSeparator);
  }

  inline bool IsRelative(const fextl::string &Path) {
    return Path.starts_with('/');
  }

  enum class CopyOptions {
    NONE,
    SKIP_EXISTING,
    OVERWRITE_EXISTING,
  };

  inline bool CopyFile(const fextl::string &From, const fextl::string &To, CopyOptions Options = CopyOptions::NONE) {
    bool DestTested = false;
    bool DestExists = false;
    if (Options == CopyOptions::SKIP_EXISTING && Exists(To.c_str())) {
      DestTested = true;
      DestExists = Exists(To.c_str());
      if (DestExists) {
        // If the destination file exists already and the skip existing flag is set then
        // return true without error.
        return true;
      }
    }

    if (!DestTested) {
      DestTested = true;
      DestExists = Exists(To.c_str());
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

  inline fextl::string LexicallyNormal(const fextl::string &Path) {
    const auto PathSize = Path.size();

    // Early exit on empty paths.
    if (PathSize == 0) {
      return {};
    }

    // Count the number of separators up front
    const auto SeparatorCount = std::count(Path.begin(), Path.end(), '/');

    // Needs to be a list so iterators aren't invalidated while erasing.
    // Maximum number of list regions will be the counted separators plus 2.
    // Multiplied by two since it allocates * 2 the size (32-bytes per element)
    // Use a small stack allocator to be more optimal.
    // Needs to be a list so iterators aren't invalidated while erasing.
    size_t DataSize = sizeof(std::string_view) * (SeparatorCount + 2) * 2;
    void *Data = alloca(DataSize);
    fextl::pmr::fixed_size_monotonic_buffer_resource mbr(Data, DataSize);
    std::pmr::polymorphic_allocator pa {&mbr};
    std::pmr::list<std::string_view> Parts{pa};
    // Calculate the expected string size while parsing to reduce allocations.
    size_t ExpectedStringSize{};

    size_t CurrentOffset{};
    do {
      auto FoundSeperator = Path.find('/', CurrentOffset);
      if (FoundSeperator == Path.npos) {
        FoundSeperator = PathSize;
      }

      const auto Begin = Path.begin() + CurrentOffset;
      const auto End = Path.begin() + FoundSeperator;
      const auto Size = End - Begin;
      ExpectedStringSize += Size;

      // If Size is zero then only insert if Parts is empty.
      // Ensures that we don't remove an initial `/` by itself
      if (Parts.empty() || Size != 0) {
        Parts.emplace_back(std::string_view(Begin, End));
      }

      if (Size == 0) {
        // If the view is empty, skip over the separator.
        FoundSeperator += 1;
      }

      CurrentOffset = FoundSeperator;
    } while (CurrentOffset != PathSize);

    size_t CurrentIterDistance{};
    for (auto iter = Parts.begin(); iter != Parts.end();) {
      if (*iter == ".") {
        // Erase '.' directory parts if not at root.
        if (CurrentIterDistance > 0) {
          // Erasing this iterator, don't increase iter distances
          iter = Parts.erase(iter);
          --ExpectedStringSize;
          continue;
        }
      }

      if (*iter == "..") {
        auto Distance = std::distance(Parts.begin(), iter);
        if (Distance > 0) {
          // If not at root then remove both this iterator and the previous one.
          // ONLY if the previous iterator is also not ".."
          //
          // If the previous iterator is '.' then /only/ erase the previous iterator.
          auto PreviousIter = iter;
          --PreviousIter;

          if (*PreviousIter == ".") {
            // Erasing the previous iterator, iterator distance has subtracted by one
            --CurrentIterDistance;
            ExpectedStringSize -= PreviousIter->size();
            Parts.erase(PreviousIter);
          }
          else if (*PreviousIter != "..") {
            // Erasing the previous iterator, iterator distance has subtracted by one
            // Also erasing current iterator, which means iterator distance also doesn't increase by one.
            --CurrentIterDistance;
            ExpectedStringSize -= PreviousIter->size() + 2;
            Parts.erase(PreviousIter);
            iter = Parts.erase(iter);
            continue;
          }
        }
      }

      // Interator distance increased by one.
      ++CurrentIterDistance;
      ++iter;
    }

    fextl::string OutputPath{};
    OutputPath.reserve(ExpectedStringSize + Parts.size());

    auto iter = Parts.begin();
    for (size_t i = 0; i < Parts.size(); ++i, ++iter) {
      auto &Part = *iter;
      OutputPath += Part;

      const bool IsFinal = (i + 1) == Parts.size();
      // If the final element is ellipses then don't apply a separator.
      // Otherwise if it is anything else, apply a separator.
      const bool IsEllipses = Part == "." || Part == "..";

      const bool NeedsSeparator =
        !IsFinal || // Needs a separator if this isn't the final part.
        (IsFinal && !IsEllipses); // Needs a separator if the final part doesn't end in `.` or `..`

      if (NeedsSeparator) {
        OutputPath += '/';
      }
    }

    return OutputPath;
  }
}
