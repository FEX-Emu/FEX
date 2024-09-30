// SPDX-License-Identifier: MIT
#define _FILE_DEFINED
struct FILE;
#define _SECIMP
#define _CRTIMP

#include <memory>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cerrno>
#include <io.h>
#include <ctype.h>
#include <wchar.h>
#include <windef.h>
#include <winternl.h>
#include <winbase.h>
#include <winerror.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <handleapi.h>
#include <fileapi.h>
#include <errhandlingapi.h>
#include <wine/debug.h>
#include "../Priv.h"

struct FILE {
  HANDLE Handle {INVALID_HANDLE_VALUE};
  int FileHandle {-1};
  bool Append {false};

  FILE(HANDLE Handle, int FileHandle, bool Append)
    : Handle {Handle}
    , FileHandle {FileHandle}
    , Append {Append} {}
};

namespace {
std::mutex FileTableLock;
std::vector<std::unique_ptr<FILE>> OpenFileTable;


int ErrnoReturn(int Value) {
  errno = Value;
  return -1;
}

DWORD OpenFlagToAccess(int OpenFlag) {
  if (OpenFlag & _O_RDONLY) {
    return GENERIC_READ;
  }
  if (OpenFlag & _O_WRONLY) {
    return GENERIC_WRITE;
  }
  if (OpenFlag & _O_RDWR) {
    return GENERIC_READ | GENERIC_WRITE;
  }
  return 0;
}

DWORD OpenFlagToCreation(int OpenFlag) {
  if ((OpenFlag & (_O_TRUNC | _O_CREAT)) == (_O_TRUNC | _O_CREAT)) {
    return CREATE_ALWAYS;
  }
  if ((OpenFlag & (_O_EXCL | _O_CREAT)) == (_O_EXCL | _O_CREAT)) {
    return CREATE_NEW;
  }
  if (OpenFlag & _O_TRUNC) {
    return TRUNCATE_EXISTING;
  }
  if (OpenFlag & _O_CREAT) {
    return OPEN_ALWAYS;
  }
  return OPEN_EXISTING;
}

int AllocateFile(std::unique_ptr<FILE>&& File) {
  std::scoped_lock Lock {FileTableLock};
  auto It = std::find(OpenFileTable.begin(), OpenFileTable.end(), nullptr);
  if (It == OpenFileTable.end()) {
    It = OpenFileTable.emplace(OpenFileTable.end(), std::move(File));
  } else {
    *It = std::move(File);
  }
  size_t Idx = std::distance(OpenFileTable.begin(), It);
  if (Idx >= std::numeric_limits<int>::max()) {
    std::terminate();
  }
  (*It)->FileHandle = static_cast<int>(Idx);
  return (*It)->FileHandle;
}

FILE* GetFile(int FileHandle) {
  std::scoped_lock Lock {FileTableLock};
  return OpenFileTable[FileHandle].get();
}

void RemoveFile(int FileHandle) {
  std::scoped_lock Lock {FileTableLock};
  OpenFileTable[FileHandle].reset();
}

DWORD OriginToMoveMethod(int Origin) {
  switch (Origin) {
  case SEEK_SET: return FILE_BEGIN;
  case SEEK_CUR: return FILE_CURRENT;
  case SEEK_END: return FILE_END;
  }
  UNIMPLEMENTED();
}
} // namespace

// io.h File Operatons
DLLEXPORT_FUNC(int, _wopen, (const wchar_t* Filename, int OpenFlag, ...)) {
  DWORD Attrs = 0;
  if (OpenFlag & _O_CREAT) {
    va_list VA;
    int PermMode;
    va_start(VA, OpenFlag);
    PermMode = va_arg(VA, int);
    va_end(VA);
    if (!(PermMode & _S_IWRITE)) {
      Attrs = FILE_ATTRIBUTE_READONLY;
    }
  }
  HANDLE Handle = CreateFileW(Filename, OpenFlagToAccess(OpenFlag), FILE_SHARE_READ, nullptr, OpenFlagToCreation(OpenFlag), Attrs, nullptr);
  if (Handle != INVALID_HANDLE_VALUE) {
    return AllocateFile(std::make_unique<FILE>(Handle, -1, OpenFlag & _O_APPEND));
  }

  if (GetLastError() == ERROR_FILE_EXISTS) {
    return ErrnoReturn(EEXIST);
  }
  if (GetLastError() == ERROR_FILE_NOT_FOUND) {
    return ErrnoReturn(ENOENT);
  }
  if (GetLastError() == ERROR_ACCESS_DENIED) {
    return ErrnoReturn(EACCES);
  }
  return ErrnoReturn(ENOENT);
}

DLLEXPORT_FUNC(int, _open, (const char* Filename, int OpenFlag, ...)) {
  UNICODE_STRING FilenameW;
  if (!RtlCreateUnicodeStringFromAsciiz(&FilenameW, Filename)) {
    return ErrnoReturn(EINVAL);
  }
  int ret = 0;
  if (OpenFlag & _O_CREAT) {
    va_list VA;
    int PermMode;
    va_start(VA, OpenFlag);
    PermMode = va_arg(VA, int);
    va_end(VA);
    ret = _wopen(FilenameW.Buffer, OpenFlag, PermMode);
  } else {
    ret = _wopen(FilenameW.Buffer, OpenFlag);
  }
  RtlFreeUnicodeString(&FilenameW);
  return ret;
}

DLLEXPORT_FUNC(int, open, (const char* Filename, int OpenFlag, ...)) {
  if (OpenFlag & _O_CREAT) {
    va_list VA;
    int PermMode;
    va_start(VA, OpenFlag);
    PermMode = va_arg(VA, int);
    va_end(VA);
    return _open(Filename, OpenFlag, PermMode);
  }
  return _open(Filename, OpenFlag);
}

DLLEXPORT_FUNC(int, _close, (int FileHandle)) {
  RemoveFile(FileHandle);
  return 0;
}

int close(int FileHandle) {
  return _close(FileHandle);
}

int64_t _lseeki64(int FileHandle, int64_t Offset, int Origin) {
  LARGE_INTEGER Res;
  SetFilePointerEx(GetFile(FileHandle)->Handle, LARGE_INTEGER {.QuadPart = Offset}, &Res, OriginToMoveMethod(Origin));
  return Res.QuadPart;
}

int64_t _telli64(int FileHandle) {
  LARGE_INTEGER Res;
  SetFilePointerEx(GetFile(FileHandle)->Handle, LARGE_INTEGER {}, &Res, FILE_CURRENT);
  return Res.QuadPart;
}

DLLEXPORT_FUNC(int, _read, (int FileHandle, void* DstBuf, unsigned int MaxCharCount)) {
  DWORD Read;
  ReadFile(GetFile(FileHandle)->Handle, DstBuf, MaxCharCount, &Read, nullptr);
  return static_cast<int>(Read);
}

int read(int FileHandle, void* DstBuf, unsigned int MaxCharCount) {
  return _read(FileHandle, DstBuf, MaxCharCount);
}

DLLEXPORT_FUNC(int, _write, (int FileHandle, const void* Buf, unsigned int MaxCharCount)) {
  DWORD Written;
  FILE* File = GetFile(FileHandle);
  if (File->Append) {
    SetFilePointerEx(File->Handle, LARGE_INTEGER {}, nullptr, FILE_END);
  }
  WriteFile(File->Handle, Buf, MaxCharCount, &Written, nullptr);
  return static_cast<int>(Written);
}

int write(int FileHandle, const void* Buf, unsigned int MaxCharCount) {
  return _write(FileHandle, Buf, MaxCharCount);
}

DLLEXPORT_FUNC(int, _isatty, (int _FileHandle)) {
  return 0;
}

DLLEXPORT_FUNC(intptr_t, _get_osfhandle, (int _FileHandle)) {
  UNIMPLEMENTED();
}

namespace {
template<typename TStr, typename TChar, TStr (*StrchrFunc)(TStr, TChar)>
int ModeToOpenFlag(TStr Mode) {
  int OpenFlag = 0;
  if (StrchrFunc(Mode, 'a')) {
    OpenFlag |= _O_RDWR | _O_CREAT | _O_APPEND;
  } else if (StrchrFunc(Mode, 'r')) {
    if (StrchrFunc(Mode, '+')) {
      OpenFlag |= _O_RDWR;
    } else {
      OpenFlag |= _O_RDONLY;
    }
  } else {
    OpenFlag |= _O_RDWR | _O_CREAT | _O_TRUNC;
  }
  if (StrchrFunc(Mode, 'x')) {
    OpenFlag |= _O_EXCL;
  }
  return OpenFlag;
}
} // namespace

// stdio.h File Operations
DLLEXPORT_FUNC(FILE*, _wfopen, (const wchar_t* __restrict__ Filename, const wchar_t* __restrict__ Mode)) {
  int OpenFlag = ModeToOpenFlag<const wchar_t*, wchar_t, &wcschr>(Mode);
  int Ret = _wopen(Filename, OpenFlag, _S_IWRITE | _S_IREAD);
  if (Ret == -1) {
    return nullptr;
  }
  return GetFile(Ret);
}

FILE* fopen(const char* __restrict__ Filename, const char* __restrict__ Mode) {
  int OpenFlag = ModeToOpenFlag<const char*, int, &strchr>(Mode);
  int Ret = _open(Filename, OpenFlag, _S_IWRITE | _S_IREAD);
  if (Ret == -1) {
    return nullptr;
  }
  return GetFile(Ret);
}

FILE* fdopen(int _FileHandle, const char* _Mode) {
  UNIMPLEMENTED();
}

int fclose(FILE* File) {
  RemoveFile(File->FileHandle);
  return 0;
}

DLLEXPORT_FUNC(int, _fseeki64, (FILE * File, _off64_t Offset, int Origin)) {
  SetFilePointerEx(File->Handle, LARGE_INTEGER {.QuadPart = Offset}, nullptr, OriginToMoveMethod(Origin));
  return 0;
}

int fseek(FILE* File, long Offset, int Origin) {
  return _fseeki64(File, Offset, Origin);
}

DLLEXPORT_FUNC(_off64_t, _ftelli64, (FILE * File)) {
  LARGE_INTEGER Res;
  SetFilePointerEx(File->Handle, LARGE_INTEGER {}, &Res, FILE_CURRENT);
  return Res.QuadPart;
}

long ftell(FILE* File) {
  return static_cast<long>(_ftelli64(File));
}

size_t fread(void* __restrict__ DstBuf, size_t ElementSize, size_t Count, FILE* __restrict__ File) {
  DWORD Read;
  ReadFile(File->Handle, DstBuf, ElementSize * Count, &Read, nullptr);
  return static_cast<size_t>(Read);
}

size_t fwrite(const void* __restrict__ Str, size_t Size, size_t Count, FILE* __restrict__ File) {
  DWORD Written;
  if (File->Append) {
    SetFilePointerEx(File->Handle, LARGE_INTEGER {}, nullptr, FILE_END);
  }
  WriteFile(File->Handle, Str, Size * Count, &Written, nullptr);
  return static_cast<size_t>(Written);
}

void setbuf(FILE* __restrict__ _File, char* __restrict__ _Buffer) {
  UNIMPLEMENTED();
}

int fflush(FILE* _File) {
  UNIMPLEMENTED();
}

int fprintf(FILE* __restrict__, const char* __restrict__, ...) {
  UNIMPLEMENTED();
}

int vfprintf(FILE* __restrict__, const char* __restrict__, va_list) {
  UNIMPLEMENTED();
}

int ungetc(int _Ch, FILE* _File) {
  UNIMPLEMENTED();
}

wint_t fgetwc(FILE* _File) {
  UNIMPLEMENTED();
}

wint_t fputwc(wchar_t _Ch, FILE* _File) {
  UNIMPLEMENTED();
}

int fputc(int _Ch, FILE* _File) {
  UNIMPLEMENTED();
}

int fputs(const char* __restrict__ _Str, FILE* __restrict__ _File) {
  UNIMPLEMENTED();
}

int getc(FILE* _File) {
  UNIMPLEMENTED();
}

void _lock_file(FILE* _File) {
  UNIMPLEMENTED();
}

wint_t ungetwc(wint_t _Ch, FILE* _File) {
  UNIMPLEMENTED();
}

void _unlock_file(FILE* _File) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(FILE*, __acrt_iob_func, (unsigned index)) {
  return nullptr;
}

DLLEXPORT_FUNC(int, _fileno, (FILE * _File)) {
  UNIMPLEMENTED();
}

int access(const char* Path, int AccessMode) {
  UNICODE_STRING PathW;
  if (!RtlCreateUnicodeStringFromAsciiz(&PathW, Path)) {
    return ErrnoReturn(EINVAL);
  }

  UNICODE_STRING NTPath;
  bool Success = RtlDosPathNameToNtPathName_U(PathW.Buffer, &NTPath, nullptr, nullptr);
  RtlFreeUnicodeString(&PathW);
  if (!Success) {
    return ErrnoReturn(EINVAL);
  }

  OBJECT_ATTRIBUTES ObjAttributes;
  InitializeObjectAttributes(&ObjAttributes, &NTPath, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

  FILE_BASIC_INFORMATION Info;
  Success = !NtQueryAttributesFile(&ObjAttributes, &Info);
  RtlFreeUnicodeString(&NTPath);

  if (!Success) {
    return ErrnoReturn(ENOENT);
  }

  if ((AccessMode & W_OK) && (Info.FileAttributes & FILE_ATTRIBUTE_READONLY)) {
    return ErrnoReturn(EACCES);
  }

  return 0;
}

int rename(const char* _OldFilename, const char* _NewFilename) {
  UNIMPLEMENTED();
}
