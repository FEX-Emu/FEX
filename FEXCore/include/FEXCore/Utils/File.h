// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/Utils/EnumOperators.h>
#include "FEXCore/Utils/LogManager.h"

#include <chrono>
#include <thread>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
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

  File(const char* Filepath, FileModes Modes, bool Seekable = true)
    : Seekable {Seekable} {
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

    ShouldClose = IsValidHandle;
  }

  FileHandleType GetHandle() {
    return Handle;
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
    if (!Seekable) {
      LOGMAN_THROW_A_FMT(false, "Can't use non-positioned ops on a non-seekable file!");
      return -1;
    }
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
    if (!Seekable) {
      LOGMAN_THROW_A_FMT(false, "Can't use non-positioned ops on a non-seekable file!");
      return -1;
    }
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

  ssize_t PRead(void* Buffer, size_t Bytes, uint64_t Offset) {
    if (Seekable) {
      LOGMAN_THROW_A_FMT(false, "Can't use positioned ops on a seekable file!");
      return -1;
    }
#ifndef _WIN32
    return pread(Handle, Buffer, Bytes, Offset);
#else
    DWORD BytesRead {};
    OVERLAPPED Overlapped {};
    Overlapped.Offset = static_cast<DWORD>(Offset);
    Overlapped.OffsetHigh = static_cast<DWORD>(Offset >> 32);
    auto Result = ReadFile(Handle, Buffer, Bytes, &BytesRead, &Overlapped);
    if (Result) {
      return BytesRead;
    }
    // Some error, match Linux side.
    return -1;
#endif
  }

  ssize_t PWrite(const void* Buffer, size_t Bytes, uint64_t Offset) {
    if (Seekable) {
      LOGMAN_THROW_A_FMT(false, "Can't use positioned ops on a seekable file!");
      return -1;
    }
#ifndef _WIN32
    return pwrite(Handle, Buffer, Bytes, Offset);
#else
    DWORD BytesWritten {};
    OVERLAPPED Overlapped {};
    Overlapped.Offset = static_cast<DWORD>(Offset);
    Overlapped.OffsetHigh = static_cast<DWORD>(Offset >> 32);
    auto Result = WriteFile(Handle, Buffer, Bytes, &BytesWritten, &Overlapped);
    if (Result) {
      return BytesWritten;
    }
    // Some error, match Linux side.
    return -1;
#endif
  }

  bool Lock(uint32_t TimeoutMS) {
    for (uint32_t i = 0;; ++i) {
      if (TryLock()) {
        return true;
      }
      if (i >= TimeoutMS) {
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  bool Unlock() {
    if (!Locked) {
      return false; // we could return true here :thonk:
    }
#ifndef _WIN32
    if (flock(Handle, LOCK_UN) == -1) {
      return false;
    }
#else
    OVERLAPPED Overlapped {};
    Overlapped.Offset = static_cast<DWORD>(LOCK_SENTINEL_OFFSET);
    Overlapped.OffsetHigh = static_cast<DWORD>(LOCK_SENTINEL_OFFSET >> 32);
    if (!UnlockFileEx(Handle, 0, 1, 0, &Overlapped)) {
      return false;
    }
#endif
    Locked = false;
    return true;
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

  ssize_t Size() {
#ifndef _WIN32
    struct stat st;
    if (fstat(Handle, &st) != 0) {
      return -1;
    }
    return st.st_size;
#else
    LARGE_INTEGER FileSize;
    if (!GetFileSizeEx(Handle, &FileSize)) {
      return -1;
    }
    return FileSize.QuadPart;
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
    if (!Seekable) {
      LOGMAN_THROW_A_FMT(false, "Can't use non-positioned ops on a non-seekable file!");
      return -1;
    }
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

  File(FileHandleType Handle, bool ShouldClose, bool Seekable = true)
    : ShouldClose {ShouldClose}
    , IsValidHandle {true}
    , Handle {Handle}
    , Seekable {Seekable} {}
private:
  bool TryLock() {
    if (Locked) {
      return true;
    }
#ifndef _WIN32
    if (flock(Handle, LOCK_EX | LOCK_NB) == -1) {
      return false;
    }
#else
    // mimic posix advisory-only lock by locking some unattainably-high bit
    OVERLAPPED Overlapped {};
    Overlapped.Offset = static_cast<DWORD>(LOCK_SENTINEL_OFFSET);
    Overlapped.OffsetHigh = static_cast<DWORD>(LOCK_SENTINEL_OFFSET >> 32);
    if (!LockFileEx(Handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &Overlapped)) {
      return false;
    }
#endif
    Locked = true;
    return true;
  }

  bool ShouldClose {};
  bool IsValidHandle {};

  FileHandleType Handle {};
  bool Seekable = true;
  bool Locked = false;
#ifndef _WIN32
  static constexpr int DEFAULT_USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;

  static uint32_t TranslateModes(FileModes Modes) {
    const auto IsRead = (Modes & FileModes::READ) == FileModes::READ;
    const auto IsWrite = (Modes & FileModes::WRITE) == FileModes::WRITE;

    uint32_t Mode {};
    if (IsRead && IsWrite) {
      Mode |= O_RDWR;
    } else {
      if (IsRead) {
        Mode |= O_RDONLY;
      } else if (IsWrite) {
        Mode |= O_WRONLY;
      }
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
  static constexpr uint64_t LOCK_SENTINEL_OFFSET = 1ULL << 62;

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
      if ((Modes & FileModes::TRUNCATE) == FileModes::TRUNCATE) {
        Disp.CreationFlag = CREATE_ALWAYS;
      } else {
        Disp.CreationFlag = OPEN_ALWAYS;
      }
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
