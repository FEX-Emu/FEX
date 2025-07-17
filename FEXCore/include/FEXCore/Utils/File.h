// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/Utils/EnumOperators.h>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef ERROR
#endif

namespace FEXCore::File {
enum class FileModes : uint32_t {
  READ = (1U << 0),
  WRITE = (1U << 1),
  CREATE = (1U << 2),
  TRUNCATE = (1U << 3),
};

enum class SeekOp {
  BEGIN,
  CURRENT,
  END,
};

FEX_DEF_NUM_OPS(FileModes)

class File final {
public:
#ifndef _WIN32
  using FileHandleType = int;
#else
  using FileHandleType = HANDLE;
#endif

  File() = default;

  File(const char* Filepath, FileModes Modes) {
#ifndef _WIN32
    auto Disp = TranslateModes(Modes);
    Handle = open(Filepath, Disp, DEFAULT_USER_PERMS);
    IsValidHandle = Handle != -1;
#else
    auto Disp = TranslateModes(Modes);
    if (Disp.CreationFlag == OPEN_ALWAYS && Disp.TruncateOnExist) {
      // If Open + Truncate then try to open with truncate behaviour first.
      Handle = CreateFileA(Filepath, Disp.Access, DEFAULT_SHARE_MODE, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (Handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND) {
        // File didn't exist, just open.
        Handle = CreateFileA(Filepath, Disp.Access, DEFAULT_SHARE_MODE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
      }
    } else {
      Handle = CreateFileA(Filepath, Disp.Access, DEFAULT_SHARE_MODE, nullptr, Disp.CreationFlag, FILE_ATTRIBUTE_NORMAL, nullptr);
    }
    IsValidHandle = Handle != INVALID_HANDLE_VALUE;
#endif
  }

  /**
   * @brief Write Bytes to File
   *
   * @param Buffer The buffer to write.
   * @param Bytes The number of bytes to write.
   *
   * @return The number of bytes actually written or -1 on error.
   */
  ssize_t Write(const void* Buffer, size_t Bytes) {
#ifndef _WIN32
    return write(Handle, Buffer, Bytes);
#else
    DWORD BytesWritten {};
    auto Result = WriteFile(Handle, Buffer, Bytes, &BytesWritten, nullptr);
    if (Result) {
      return BytesWritten;
    }
    // Some error, match Linux side.
    return -1;
#endif
  }

  ssize_t Write(const std::string_view Data) {
    return Write(Data.data(), Data.size());
  }

  /**
   * @brief Read at most Bytes in to the buffer.
   *
   * @param Buffer The buffer where the data is read in to.
   * @param Bytes The size of the buffer.
   *
   * @return The number of bytes read or -1 on error.
   */
  ssize_t Read(void* Buffer, size_t Bytes) {
#ifndef _WIN32
    return read(Handle, Buffer, Bytes);
#else
    DWORD BytesRead {};
    auto Result = ReadFile(Handle, Buffer, Bytes, &BytesRead, nullptr);
    if (Result) {
      return BytesRead;
    }
    // Some error, match Linux side.
    return -1;
#endif
  }

  ~File() {
    if (!IsValidHandle) {
      return;
    }
    if (!ShouldClose) {
      return;
    }
#ifndef _WIN32
    close(Handle);
#else
    CloseHandle(Handle);
#endif
  }

  /**
   * @brief Gets a File object that points to stdout
   */
  static File GetStdOUT() {
#ifndef _WIN32
    return File(STDOUT_FILENO, false);
#else
    return File(GetStdHandle(STD_OUTPUT_HANDLE), false);
#endif
  }

  /**
   * @brief Gets a File object that points to stderr
   */
  static File GetStdERR() {
#ifndef _WIN32
    return File(STDERR_FILENO, false);
#else
    return File(GetStdHandle(STD_ERROR_HANDLE), false);
#endif
  }

  /**
   * @brief Returns if the file handle is valid.
   */
  bool IsValid() const {
    return IsValidHandle;
  }

  /**
   * @brief Flush the file contents to the output file backing.
   *
   * @return True if the flush occured.
   */
  bool Flush() {
#ifndef _WIN32
    return fsync(Handle) == 0;
#else
    return FlushFileBuffers(Handle);
#endif
  }

  /**
   * @brief Seek the file pointer location.
   *
   * @param Distance The distance to travel.
   * @param Op The operation from where to start the travel.
   *
   * @return The current file pointer location or -1.
   */
  ssize_t Seek(ssize_t Distance, SeekOp Op) {
#ifndef _WIN32
    return lseek(Handle, Distance, TranslateSeek(Op));
#else
    LARGE_INTEGER NewDistance {.QuadPart = Distance};
    LARGE_INTEGER NewPointer;
    auto Result = SetFilePointerEx(Handle, NewDistance, &NewPointer, TranslateSeek(Op));
    if (Result) {
      return NewPointer.QuadPart;
    }
    // Some error, match Linux side.
    return -1;
#endif
  }

protected:

  File(FileHandleType Handle, bool ShouldClose)
    : ShouldClose {ShouldClose}
    , IsValidHandle {true}
    , Handle {Handle} {}
private:
  bool ShouldClose {};
  bool IsValidHandle {};

  FileHandleType Handle {};
#ifndef _WIN32
  static constexpr int DEFAULT_USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;

  static uint32_t TranslateModes(FileModes Modes) {
    uint32_t Mode {};
    if ((Modes & FileModes::READ) == FileModes::READ) {
      Mode |= O_RDONLY;
    }
    if ((Modes & FileModes::WRITE) == FileModes::WRITE) {
      Mode |= O_WRONLY;
    }
    if ((Modes & FileModes::CREATE) == FileModes::CREATE) {
      Mode |= O_CREAT;
    }
    if ((Modes & FileModes::TRUNCATE) == FileModes::TRUNCATE) {
      Mode |= O_TRUNC;
    }

    // Always enable CLOEXEC so that the FD is closed on execve.
    // FEXCore never wants to leak FDs across execve using this interface.
    Mode |= O_CLOEXEC;
    return Mode;
  }

  static uint32_t TranslateSeek(SeekOp Op) {
    switch (Op) {
    case SeekOp::BEGIN: return SEEK_SET;
    case SeekOp::CURRENT: return SEEK_CUR;
    case SeekOp::END: return SEEK_END;
    default: FEX_UNREACHABLE;
    }
  }
#else
  static constexpr int DEFAULT_SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  struct Disposition {
    uint32_t CreationFlag;
    uint32_t Access;
    bool TruncateOnExist;
  };
  static Disposition TranslateModes(FileModes Modes) {
    Disposition Disp {};
    if ((Modes & FileModes::READ) == FileModes::READ) {
      Disp.Access |= GENERIC_READ;
    }
    if ((Modes & FileModes::WRITE) == FileModes::WRITE) {
      Disp.Access |= GENERIC_WRITE;
    }
    if ((Modes & FileModes::CREATE) == FileModes::CREATE) {
      Disp.CreationFlag = CREATE_ALWAYS;
    } else {
      Disp.CreationFlag = OPEN_ALWAYS;
    }

    if ((Modes & FileModes::TRUNCATE) == FileModes::TRUNCATE) {
      Disp.TruncateOnExist = true;
    }

    return Disp;
  }

  static uint32_t TranslateSeek(SeekOp Op) {
    switch (Op) {
    case SeekOp::BEGIN: return FILE_BEGIN;
    case SeekOp::CURRENT: return FILE_CURRENT;
    case SeekOp::END: return FILE_END;
    default: FEX_UNREACHABLE;
    }
  }
#endif
};
} // namespace FEXCore::File
