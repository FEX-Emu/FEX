#include "Interface/Context/Context.h"
#include "Interface/Core/CodeSerialize/CodeSerialize.h"

#include <FEXCore/Config/Config.h>

#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <string>
#include <sys/uio.h>
#include <sys/mman.h>
#include <xxhash.h>

namespace FEXCore::CodeSerialize {
  void CodeSerializeService::DoClosure(AddrToFileMapType::iterator it) {
    if (it->first == ~0ULL) {
      // Don't do closure on canary
      return;
    }
    // Can't delete if there is a relocation IN the section OR if there is a relocation in a different section pointing to ANOTHER section
    // Common when a code block relocates in to a data block
    //
    bool Serialized = false;
    if (it->second->CodeHeader.NumCodeEntries > 0 ||
        it->second->CodeHeader.NumRelocationsTo > 0)
    {
      Serialized = true;
    }

    // Just need to ensure we still track the ranges that relocate INTO these objects
    if (!Serialized) {
      std::filesystem::remove(it->second->SourceCodePath);
    }
  }

  bool CodeSerializeService::AppendSerializeDataFD(int fd, AddrToFileMapType::iterator it, CodeSerializationData *CodeData, AOTData *Data) {
    if (1) {
      std::vector<iovec> WriteData(Data->Relocations.size() + 2);

      // Code Data Header
      WriteData[0].iov_base = CodeData;
      WriteData[0].iov_len = sizeof(*CodeData);

      // Code Data
      WriteData[1].iov_base = Data->HostCodeBegin;
      WriteData[1].iov_len = Data->HostCodeLength;

      // Relocations
      uint64_t RelocationSize{};
      for (size_t i = 0; i < Data->Relocations.size(); ++i) {
        switch (Data->Relocations[i].Header.Type) {
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_MOVE:
            WriteData[i + 2].iov_base = &Data->Relocations[i].NamedSymbolMove;
            WriteData[i + 2].iov_len = sizeof(Data->Relocations[i].NamedSymbolMove);
            RelocationSize += sizeof(Data->Relocations[i].NamedSymbolMove);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL:
            WriteData[i + 2].iov_base = &Data->Relocations[i].NamedSymbolLiteral;
            WriteData[i + 2].iov_len = sizeof(Data->Relocations[i].NamedSymbolLiteral);
            RelocationSize += sizeof(Data->Relocations[i].NamedSymbolLiteral);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE:
            WriteData[i + 2].iov_base = &Data->Relocations[i].NamedThunkMove;
            WriteData[i + 2].iov_len = sizeof(Data->Relocations[i].NamedThunkMove);
            RelocationSize += sizeof(Data->Relocations[i].NamedThunkMove);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE:
            // Incoming RELOC_GUEST_RIP_MOVE from the JIT needs to offset the current shared library base then
            // add the original base.
            // On a fresh cache: it->first == OriginalBase
            // On a baked cache: it->first != OriginalBase
            //
            // This keeps all of our relocations within the same address space, defined by when the cache was baked first.
            Data->Relocations[i].GuestRIPMove.GuestRIP -= it->first;
            Data->Relocations[i].GuestRIPMove.GuestRIP += it->second->CodeHeader.OriginalBase;

            WriteData[i + 2].iov_base = &Data->Relocations[i].GuestRIPMove;
            WriteData[i + 2].iov_len = sizeof(Data->Relocations[i].GuestRIPMove);
            RelocationSize += sizeof(Data->Relocations[i].GuestRIPMove);
            break;
          default:
            LogMan::Msg::AFmt("Unknown relocation type: {}", static_cast<int>(Data->Relocations[i].Header.Type));
            break;
        }
      }

      CodeData->RelocationSize = RelocationSize;

      // Append file contents with RWF_APPEND
      pwritev2(fd, WriteData.data(), WriteData.size(), 0, RWF_APPEND);
    }
    else {
      size_t DataSize{};
      size_t HeaderOffset{};
      size_t CodeOffset{};
      size_t RelocationsOffset{};

      // Code header
      HeaderOffset = DataSize;
      DataSize += sizeof(*CodeData);

      // Code Data
      CodeOffset = DataSize;
      DataSize += Data->HostCodeLength;

      // Relocations
      RelocationsOffset = DataSize;

      for (size_t i = 0; i < Data->Relocations.size(); ++i) {
        switch (Data->Relocations[i].Header.Type) {
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_MOVE:
            DataSize += sizeof(Data->Relocations[i].NamedSymbolMove);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL:
            DataSize += sizeof(Data->Relocations[i].NamedSymbolLiteral);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE:
            DataSize += sizeof(Data->Relocations[i].NamedThunkMove);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE:
            DataSize += sizeof(Data->Relocations[i].GuestRIPMove);
            break;
          default:
            LogMan::Msg::AFmt("Unknown relocation type: {}", static_cast<int>(Data->Relocations[i].Header.Type));
            break;
        }
      }

      std::vector<char> RawData(DataSize);

      // Code Data header
      memcpy(&RawData.at(HeaderOffset), CodeData, sizeof(*CodeData));

      // Code Data
      memcpy(&RawData.at(CodeOffset), Data->HostCodeBegin, Data->HostCodeLength);

      // Relocations

      size_t RelocationOffset2{};
      for (size_t i = 0; i < Data->Relocations.size(); ++i) {
        switch (Data->Relocations[i].Header.Type) {
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_MOVE:
            memcpy(&RawData.at(RelocationsOffset + RelocationOffset2), &Data->Relocations[i].NamedSymbolMove, sizeof(Data->Relocations[i].NamedSymbolMove));
            RelocationOffset2 += sizeof(Data->Relocations[i].NamedSymbolMove);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL:
            memcpy(&RawData.at(RelocationsOffset + RelocationOffset2), &Data->Relocations[i].NamedSymbolLiteral, sizeof(Data->Relocations[i].NamedSymbolLiteral));
            RelocationOffset2 += sizeof(Data->Relocations[i].NamedSymbolLiteral);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE:
            memcpy(&RawData.at(RelocationsOffset + RelocationOffset2), &Data->Relocations[i].NamedThunkMove, sizeof(Data->Relocations[i].NamedThunkMove));
            RelocationOffset2 += sizeof(Data->Relocations[i].NamedThunkMove);
            break;
          case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
            memcpy(&RawData.at(RelocationsOffset + RelocationOffset2), &Data->Relocations[i].GuestRIPMove, sizeof(Data->Relocations[i].GuestRIPMove));

            auto GuestRIPMove = reinterpret_cast<decltype(Data->Relocations[i].GuestRIPMove)*>(&RawData.at(RelocationsOffset + RelocationOffset2));

            // Incoming RELOC_GUEST_RIP_MOVE from the JIT needs to offset the current shared library base then
            // add the original base.
            // On a fresh cache: it->first == OriginalBase
            // On a baked cache: it->first != OriginalBase
            //
            // This keeps all of our relocations within the same address space, defined by when the cache was baked first.
            GuestRIPMove->GuestRIP -= it->first;
            GuestRIPMove->GuestRIP += it->second->CodeHeader.OriginalBase;

            RelocationOffset2 += sizeof(Data->Relocations[i].GuestRIPMove);
            break;
          }
          default:
            LogMan::Msg::AFmt("Unknown relocation type: {}", static_cast<int>(Data->Relocations[i].Header.Type));
            break;
        }
      }
      iovec vec {
        .iov_base = &RawData.at(0),
        .iov_len = DataSize
      };

      // Append file contents with RWF_APPEND
      pwritev2(fd, &vec, 1, 0, RWF_APPEND);
    }

    return true;
  }

  void CodeSerializeService::AppendSerializeData(AddrToFileMapType::iterator it, CodeSerializationData *CodeData, AOTData *Data) {
    int fd{};
    if (it->second->CurrentSerializeFD == -1) {
      fd = OpenFDForAppend(it->second->SourceCodePath.c_str());
      if (fd == -1) {
        // Couldn't open the file. Did the user delete it while we were serializing?
        // Throw this serialization data away and no longer serialize to this region
        it->second->StillSerializing = false;
        return;
      }

      // Since we are opening this, get the FD lock
      GetFDLock(fd, F_WRLCK);
      it->second->CurrentSerializeFD = fd;
    }
    else {
      fd = it->second->CurrentSerializeFD;
    }

    CodeSerializationHeader CodeHeader;
    ReadHeader(fd, &CodeHeader);

    // Only serialize the data if the config matches
    if (CodeHeader.Config == NamedRegionHandling.GetDefaultSerializationConfig()) {
      // Copy over the code header incase of changes
      it->second->CodeHeader = CodeHeader;

      // Append the data
      if (AppendSerializeDataFD(fd, it, CodeData, Data)) {
        // Update the header
        it->second->CodeHeader.TotalCodeSize += Data->HostCodeLength;
        it->second->CodeHeader.TotalRelocationCount += Data->Relocations.size();
        it->second->CodeHeader.NumCodeEntries++;
        WriteHeader(fd, &it->second->CodeHeader);
      }
    }
  }

  void CodeSerializeService::Serialize(AddrToFileMapType::iterator it, AOTData *Data) {
    // Thread safety here! We are returning an iterator to the map object
    // This needs the AOTIRCacheLock locked prior to coming in to the function

    // First step, check for relocations that try to escape the region.
    // XXX: Can we safely relocate guest RIP moves that escape to another known region?
    // Investigate this in the future.
    // XXX: Disabling this code causes crashes
    // XXX: Easy reproduction case is `fex-posixtest-bins/conformance/interfaces/sigaction/1-14.test`
    // Caues a crash in _start at `call    qword [rel __libc_start_main]`
    // Tries to jump to zero because of an invalid relocation
    for (auto &Reloc : Data->Relocations) {
      switch (Reloc.Header.Type) {
        case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE:
          if ((Reloc.GuestRIPMove.GuestRIP - it->first) >= it->second->Len) {
            // This relocation escapes the region. Reject the full serialization.
            return;
          }
          break;
        default: break;
      }
    }

    if (Data->GuestCodeHash == 0) {
      Data->GuestCodeHash = XXH3_64bits((void*)Data->GuestRIP, Data->GuestCodeLength);
    }

    size_t NumRelocations = Data->Relocations.size();
    CodeSerializationData CodeData
    {
      // Data->GuestRIP coming in will always be the current address mapping of the region
      // This is necessary otherwise xxhash above will crash or generate invalid hashes
      .RIPOffset = Data->GuestRIP - it->first,
      .HostCodeHash = Data->HostCodeHash,
      .HostCodeLength = Data->HostCodeLength,
      .NumRelocations = NumRelocations,
      .GuestCodeHash = Data->GuestCodeHash,
      .GuestCodeLength = Data->GuestCodeLength,
    };

    static int NumSerialized = 0;
    AOTLOG("Serialized {} sections", NumSerialized++);

    AppendSerializeData(it, &CodeData, Data);
  }

  void CodeSerializeService::SerializeHandler(std::unique_ptr<AOTData> Data) {
    // Get iterator from the AOTData
    auto it = Data->MapIter;

    if (it != AddrToFile.end() && it->second->StillSerializing) {
      bool DoSerialization = true;
      {
        // Check to see if we already have this symbol data
        // Search the section lookup with the region offset
        uint64_t NamedRegionOffset = Data->GuestRIP - it->first;

        auto CodeSectionIt = it->second->SectionLookupMap.find(NamedRegionOffset);
        if (CodeSectionIt != it->second->SectionLookupMap.end()) {
          // We already have this symbol, skip it.
          DoSerialization = false;
          //AOTLOG("Skipping Serializing 0x{:x} offset 0x{:x} in {}", Data->GuestRIP, Data->GuestRIP - it->first, it->second.SourceCodePath);
        }
        else {
          //AOTLOG("Found Named region but did not find a matching offset");
        }
      }

      if (DoSerialization) {
        //AOTLOG("SerializeHandler: Serializing 0x{:x} offset 0x{:x} in {}", Data->GuestRIP, Data->GuestRIP - it->first, it->second.filepath);
        Serialize(it, Data.get());
      }
    }

    // Decrement the named regions ref counter
    Data->ObjectJobRefCountMutexPtr->unlock_shared();
    if (it->second->CurrentSerializeFD != -1 && Data->ObjectJobRefCountMutexPtr->try_lock()) {
      // Close the current serialization FD
      // This frees the file lock
      close(it->second->CurrentSerializeFD);
      it->second->CurrentSerializeFD = -1;
      Data->ObjectJobRefCountMutexPtr->unlock();
    }

    // Decrement the share counter
    Data->ThreadRefCountAOT->unlock_shared();
  }
}
