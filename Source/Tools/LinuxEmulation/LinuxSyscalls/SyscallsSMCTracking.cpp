// SPDX-License-Identifier: MIT
/*
$info$
category: LinuxSyscalls ~ Linux syscall emulation, marshaling and passthrough
tags: LinuxSyscalls|common
desc: SMC/MMan Tracking
$end_info$
*/

#include "Common/FDUtils.h"

#include <filesystem>
#include <sys/shm.h>
#include <sys/mman.h>

#include "LinuxSyscalls/Syscalls.h"

#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/SignalScopeGuards.h>
#include <FEXCore/Utils/TypeDefines.h>

namespace FEX::HLE {
// SMC interactions
bool SyscallHandler::HandleSegfault(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) {
  const auto FaultAddress = (uintptr_t)((siginfo_t*)info)->si_addr;

  {
    // Can't use the deferred signal lock in the SIGSEGV handler.
    auto lk = FEXCore::MaskSignalsAndLockMutex<std::shared_lock>(_SyscallHandler->VMATracking.Mutex);

    auto VMATracking = &_SyscallHandler->VMATracking;

    // If the write spans two pages, they will be flushed one at a time (generating two faults)
    auto Entry = VMATracking->FindVMAEntry(FaultAddress);

    // If an untracked address, or the mapping wasn't writable, it can't be handled here
    if (Entry == VMATracking->VMAs.end() || !Entry->second.Prot.Writable) {
      return false;
    }

    auto FaultBase = FEXCore::AlignDown(FaultAddress, FEXCore::Utils::FEX_PAGE_SIZE);

    auto UnprotectRegionCallback = [](uintptr_t Start, uintptr_t Length) {
      auto rv = mprotect((void*)Start, Length, PROT_READ | PROT_WRITE);
      LogMan::Throw::AFmt(rv == 0, "mprotect({}, {}) failed", Start, Length);
    };

    if (Entry->second.Flags.Shared) {
      LOGMAN_THROW_A_FMT(Entry->second.Resource, "VMA tracking error");

      auto Offset = FaultBase - Entry->first + Entry->second.Offset;

      auto VMA = Entry->second.Resource->FirstVMA;
      LOGMAN_THROW_A_FMT(VMA, "VMA tracking error");

      // Flush all mirrors, remap the page writable as needed
      do {
        if (VMA->Offset <= Offset && (VMA->Offset + VMA->Length) > Offset) {
          auto FaultBaseMirrored = Offset - VMA->Offset + VMA->Base;

          if (VMA->Prot.Writable) {
            _SyscallHandler->TM.InvalidateGuestCodeRange(Thread, FaultBaseMirrored, FEXCore::Utils::FEX_PAGE_SIZE, UnprotectRegionCallback);
          } else {
            _SyscallHandler->TM.InvalidateGuestCodeRange(Thread, FaultBaseMirrored, FEXCore::Utils::FEX_PAGE_SIZE);
          }
        }
      } while ((VMA = VMA->ResourceNextVMA));
    } else {
      _SyscallHandler->TM.InvalidateGuestCodeRange(Thread, FaultBase, FEXCore::Utils::FEX_PAGE_SIZE, UnprotectRegionCallback);
    }

    FEXCORE_PROFILE_INSTANT_INCREMENT(Thread, AccumulatedSMCCount, 1);
    return true;
  }
}

void SyscallHandler::MarkGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) {
  const auto Base = Start & FEXCore::Utils::FEX_PAGE_MASK;
  const auto Top = FEXCore::AlignUp(Start + Length, FEXCore::Utils::FEX_PAGE_SIZE);

  {
    if (SMCChecks != FEXCore::Config::CONFIG_SMC_MTRACK) {
      return;
    }

    auto lk = FEXCore::GuardSignalDeferringSection<std::shared_lock>(VMATracking.Mutex, Thread);

    // Find the first mapping at or after the range ends, or ::end().
    // Top points to the address after the end of the range
    auto Mapping = VMATracking.VMAs.lower_bound(Top);

    while (Mapping != VMATracking.VMAs.begin()) {
      Mapping--;

      const auto MapBase = Mapping->first;
      const auto MapTop = MapBase + Mapping->second.Length;

      if (MapTop <= Base) {
        // Mapping ends before the Range start, exit
        break;
      } else {
        const auto ProtectBase = std::max(MapBase, Base);
        const auto ProtectSize = std::min(MapTop, Top) - ProtectBase;

        if (Mapping->second.Flags.Shared) {
          LOGMAN_THROW_A_FMT(Mapping->second.Resource, "VMA tracking error");

          const auto OffsetBase = ProtectBase - Mapping->first + Mapping->second.Offset;
          const auto OffsetTop = OffsetBase + ProtectSize;

          auto VMA = Mapping->second.Resource->FirstVMA;
          LOGMAN_THROW_A_FMT(VMA, "VMA tracking error");

          do {
            auto VMAOffsetBase = VMA->Offset;
            auto VMAOffsetTop = VMA->Offset + VMA->Length;
            auto VMABase = VMA->Base;

            if (VMA->Prot.Writable && VMAOffsetBase < OffsetTop && VMAOffsetTop > OffsetBase) {

              const auto MirroredBase = std::max(VMAOffsetBase, OffsetBase);
              const auto MirroredSize = std::min(OffsetTop, VMAOffsetTop) - MirroredBase;

              auto rv = mprotect((void*)(MirroredBase - VMAOffsetBase + VMABase), MirroredSize, PROT_READ);
              LogMan::Throw::AFmt(rv == 0, "mprotect({}, {}) failed", MirroredBase, MirroredSize);
            }
          } while ((VMA = VMA->ResourceNextVMA));

        } else if (Mapping->second.Prot.Writable) {
          int rv = mprotect((void*)ProtectBase, ProtectSize, PROT_READ);

          LogMan::Throw::AFmt(rv == 0, "mprotect({}, {}) failed", ProtectBase, ProtectSize);
        }
      }
    }
  }
}

// Used for AOT
FEXCore::HLE::AOTIRCacheEntryLookupResult SyscallHandler::LookupAOTIRCacheEntry(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestAddr) {
  auto lk = FEXCore::GuardSignalDeferringSection<std::shared_lock>(VMATracking.Mutex, Thread);

  // Get the first mapping after GuestAddr, or end
  // GuestAddr is inclusive
  // If the write spans two pages, they will be flushed one at a time (generating two faults)
  auto Entry = VMATracking.FindVMAEntry(GuestAddr);
  if (Entry == VMATracking.VMAs.end()) {
    return {nullptr, 0};
  }

  return {Entry->second.Resource ? Entry->second.Resource->AOTIRCacheEntry : nullptr, Entry->second.Base - Entry->second.Offset};
}

// MMan Tracking
void SyscallHandler::TrackMmap(FEXCore::Core::InternalThreadState* Thread, uint64_t addr, size_t length, int prot, int flags, int fd, off_t offset) {
  size_t Size = FEXCore::AlignUp(length, FEXCore::Utils::FEX_PAGE_SIZE);

  VMATracking::MappedResource* Resource = nullptr;

  if (!(flags & MAP_ANONYMOUS)) {
    struct stat64 buf;
    fstat64(fd, &buf);
    VMATracking::MRID mrid {buf.st_dev, buf.st_ino};

    char Tmp[PATH_MAX];
    auto PathLength = FEX::get_fdpath(fd, Tmp);

    if (PathLength != -1) {
      Tmp[PathLength] = '\0';
      auto [Iter, Inserted] = VMATracking.EmplaceMappedResource(mrid, VMATracking::MappedResource {nullptr, nullptr, 0});
      Resource = &Iter->second;

      if (Inserted) {
        Resource->AOTIRCacheEntry = CTX->LoadAOTIRCacheEntry(fextl::string(Tmp, PathLength));
        Resource->Iterator = Iter;
      }
    }
  } else if (flags & MAP_SHARED) {
    VMATracking::MRID mrid {VMATracking::SpecialDev::Anon, AnonSharedId++};

    auto [Iter, Inserted] = VMATracking.EmplaceMappedResource(mrid, VMATracking::MappedResource {nullptr, nullptr, 0});
    LOGMAN_THROW_A_FMT(Inserted == true, "VMA tracking error");
    Resource = &Iter->second;
    Resource->Iterator = Iter;
  } else {
    Resource = nullptr;
  }

  VMATracking.TrackVMARange(CTX, Resource, addr, offset, Size, VMATracking::VMAFlags::fromFlags(flags), VMATracking::VMAProt::fromProt(prot));
}

void SyscallHandler::TrackMunmap(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t length) {
  uint64_t Size = FEXCore::AlignUp(length, FEXCore::Utils::FEX_PAGE_SIZE);
  VMATracking.DeleteVMARange(CTX, reinterpret_cast<uintptr_t>(addr), Size);
}

void SyscallHandler::TrackMprotect(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t len, int prot) {
  uint64_t Size = FEXCore::AlignUp(len, FEXCore::Utils::FEX_PAGE_SIZE);

  VMATracking.ChangeProtectionFlags(reinterpret_cast<uintptr_t>(addr), Size, VMATracking::VMAProt::fromProt(prot));
}

void SyscallHandler::TrackMremap(FEXCore::Core::InternalThreadState* Thread, uint64_t OldAddress, size_t OldSize, size_t NewSize, int flags,
                                 uint64_t NewAddress) {
  OldSize = FEXCore::AlignUp(OldSize, FEXCore::Utils::FEX_PAGE_SIZE);
  NewSize = FEXCore::AlignUp(NewSize, FEXCore::Utils::FEX_PAGE_SIZE);

  const auto OldVMA = VMATracking.FindVMAEntry(OldAddress);

  const auto OldResource = OldVMA->second.Resource;
  const auto OldOffset = OldVMA->second.Offset + OldAddress - OldVMA->first;
  const auto OldFlags = OldVMA->second.Flags;
  const auto OldProt = OldVMA->second.Prot;

  LOGMAN_THROW_A_FMT(OldVMA != VMATracking.VMAs.end(), "VMA Tracking corruption");

  if (OldSize == 0) {
    // Mirror existing mapping
    // must be a shared mapping
    LOGMAN_THROW_A_FMT(OldResource != nullptr, "VMA Tracking error");
    LOGMAN_THROW_A_FMT(OldFlags.Shared, "VMA Tracking error");
    VMATracking.TrackVMARange(CTX, OldResource, NewAddress, OldOffset, NewSize, OldFlags, OldProt);
  } else {

#ifndef MREMAP_DONTUNMAP
// MREMAP_DONTUNMAP is kernel 5.7+ and might not exist
#define MREMAP_DONTUNMAP 4
#endif
    if (!(flags & MREMAP_DONTUNMAP)) {
      VMATracking.DeleteVMARange(CTX, OldAddress, OldSize, OldResource);
    }

    // Make anonymous mapping
    VMATracking.TrackVMARange(CTX, OldResource, NewAddress, OldOffset, NewSize, OldFlags, OldProt);
  }
}

void SyscallHandler::TrackShmat(FEXCore::Core::InternalThreadState* Thread, int shmid, uint64_t shmaddr, int shmflg, uint64_t Length) {
  VMATracking::MRID mrid {VMATracking::SpecialDev::SHM, static_cast<uint64_t>(shmid)};

  auto [Iter, Inserted] = VMATracking.EmplaceMappedResource(mrid, VMATracking::MappedResource {nullptr, nullptr, Length});
  auto Resource = &Iter->second;
  if (Inserted) {
    Resource->Iterator = Iter;
  }
  VMATracking.TrackVMARange(CTX, Resource, shmaddr, 0, Length, VMATracking::VMAFlags::fromFlags(MAP_SHARED), VMATracking::VMAProt::fromSHM(shmflg));
}

uint64_t SyscallHandler::TrackShmdt(FEXCore::Core::InternalThreadState* Thread, uint64_t shmaddr) {
  return VMATracking.DeleteSHMRegion(CTX, reinterpret_cast<uintptr_t>(shmaddr));
}

void SyscallHandler::TrackMadvise(FEXCore::Core::InternalThreadState* Thread, uintptr_t Base, uintptr_t Size, int advice) {
  Size = FEXCore::AlignUp(Size, FEXCore::Utils::FEX_PAGE_SIZE);
  {
    auto lk = FEXCore::GuardSignalDeferringSection(VMATracking.Mutex, Thread);
    // TODO
  }
}

} // namespace FEX::HLE
