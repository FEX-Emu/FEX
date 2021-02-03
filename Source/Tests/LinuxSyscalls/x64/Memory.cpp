#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <map>
#include <unistd.h>

#include <FEXCore/Core/Context.h>
#include <FEXCore/Config/Config.h>
#include <fstream>

struct AddrToFileEntry {
  uint64_t Start;
  uint64_t Len;
  uint64_t Offset;
  std::string fileid;
};

extern std::map<uint64_t, AddrToFileEntry> AddrToFile;

std::string get_fdpath(int fd)
{
    std::vector<char> buf(400);
    ssize_t len;

    std::string fdToName = "/proc/self/fd/" + std::to_string(fd);

    do
    {
        buf.resize(buf.size() + 100);
        len = ::readlink(fdToName.c_str(), &(buf[0]), buf.size());
    } while (buf.size() == len);

    if (len > 0)
    {
        buf[len] = '\0';
        return (std::string(&(buf[0])));
    }
    /* handle error */
    return "";
}

namespace {
  // Compression function for Merkle-Damgard construction.
  // This function is generated using the framework provided.
  #define mix(h) ({					\
        (h) ^= (h) >> 23;		\
        (h) *= 0x2127599bf4325c37ULL;	\
        (h) ^= (h) >> 47; })

  static uint64_t fasthash64(const void *buf, size_t len, uint64_t seed)
  {
    const uint64_t    m = 0x880355f21e6d1965ULL;
    const uint64_t *pos = (const uint64_t *)buf;
    const uint64_t *end = pos + (len / 8);
    const unsigned char *pos2;
    uint64_t h = seed ^ (len * m);
    uint64_t v;

    while (pos != end) {
      v  = *pos++;
      h ^= mix(v);
      h *= m;
    }

    pos2 = (const unsigned char*)pos;
    v = 0;

    switch (len & 7) {
    case 7: v ^= (uint64_t)pos2[6] << 48;
    case 6: v ^= (uint64_t)pos2[5] << 40;
    case 5: v ^= (uint64_t)pos2[4] << 32;
    case 4: v ^= (uint64_t)pos2[3] << 24;
    case 3: v ^= (uint64_t)pos2[2] << 16;
    case 2: v ^= (uint64_t)pos2[1] << 8;
    case 1: v ^= (uint64_t)pos2[0];
      h ^= mix(v);
      h *= m;
    }

    return mix(h);
  }
  #undef mix
}

std::unordered_set<std::string> LoadedModules;

namespace FEX::HLE::x64 {
  void RegisterMemory() {
    REGISTER_SYSCALL_IMPL_X64(munmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length) -> uint64_t {
      uint64_t Result = ::munmap(addr, length);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
      static FEXCore::Config::Value<bool> AOTIRLoad(FEXCore::Config::CONFIG_AOTIR_LOAD, false);
      uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags, fd, offset));
      if (Result != -1 && !(flags & MAP_ANONYMOUS)) {
        auto filename = get_fdpath(fd);

        auto base_filename = filename.substr(filename.find_last_of("/\\") + 1);

        if (base_filename.size()) {
          auto filename_hash = fasthash64(filename.c_str(), filename.size(), 0xBAADF00D);

          auto fileid = base_filename + "-" + std::to_string(filename_hash);

          //fprintf(stderrr, "mmap: %lX - %ld -> %s -> %s\n", Result, length, fileid.c_str(), filename.c_str());
          AddrToFile.insert({ Result, { Result, length, (uint64_t)offset, fileid } });

          if (AOTIRLoad() && !LoadedModules.contains(fileid)) {
            std::ifstream AOTRead(std::string(getenv("HOME")) + "/.fex-emu/aotir/" + fileid, std::ios::in | std::ios::binary);

            if (AOTRead) {
              LoadedModules.insert(fileid);
              if (FEXCore::Context::ReadAOTIR(Thread->CTX, AOTRead)) {
                LogMan::Msg::I("AOTIR Cache Dynamic Load: %s", fileid.c_str());
              }
            }
          }
        }
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mremap, [](FEXCore::Core::InternalThreadState *Thread, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mlockall, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = ::mlockall(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(munlockall, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::munlockall();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(shmat, [](FEXCore::Core::InternalThreadState *Thread, int shmid, const void *shmaddr, int shmflg) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(shmat(shmid, shmaddr, shmflg));
      SYSCALL_ERRNO();
    });
  }
}
