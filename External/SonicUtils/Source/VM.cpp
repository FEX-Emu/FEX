#include <SonicUtils/LogManager.h>
#include <SonicUtils/VM.h>

#include <memory>
#include <fcntl.h>
#include <linux/kvm.h>
#include <linux/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>

inline uint64_t AlignUp(uint64_t value, uint64_t size) {
  return value + (size - value % size) % size;
};

class Memmap {
public:
  bool AllocateSHMRegion(size_t Size);
  void DeallocateSHMRegion();
  int GetFD() { return SHMfd; }
  void *MapRegionFlags(size_t Offset, size_t Size, uint32_t flags, bool Fixed);
  void *MapRegion(size_t Offset, size_t Size, bool Fixed);

  void *MapRegionMirror(size_t BasePtr, size_t Offset, size_t Size, uint32_t flags, bool Fixed);
  void *FakeMapRegion(size_t Offset);

private:
  int SHMfd;
  size_t SHMSize;
  void *Base;
};
void *Memmap::MapRegion(size_t Offset, size_t Size, bool Fixed) {
  return MapRegionFlags(Offset, Size, PROT_READ | PROT_WRITE, Fixed);
}

void *Memmap::MapRegionFlags(size_t Offset, size_t Size, uint32_t flags, bool Fixed) {
  return mmap(0, Size, flags, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  uintptr_t PtrOffset = reinterpret_cast<uintptr_t>(Base) + Offset;
  void *Ptr = mmap(reinterpret_cast<void*>(PtrOffset), Size, flags,
      MAP_SHARED | (Fixed ? MAP_FIXED : 0), SHMfd, Offset);

  if (Ptr == MAP_FAILED) {
    LogMan::Msg::A("Failed to map memory region [0x%lx, 0x%lx) - %d", Offset, Size, Fixed);
    return nullptr;
  }
  LogMan::Msg::D("Mapped [0x%0lx, 0x%0lx) to 0x%0lx on host", Offset, Offset + Size, Ptr);

  return Ptr;
}

void *Memmap::MapRegionMirror(size_t BasePtr, size_t Offset, size_t Size, uint32_t flags, bool Fixed) {
  uintptr_t PtrOffset = reinterpret_cast<uintptr_t>(Base) + BasePtr;
  void *Ptr = mmap(reinterpret_cast<void*>(PtrOffset), Size, flags,
      MAP_SHARED | (Fixed ? MAP_FIXED : 0), SHMfd, Offset);

  if (Ptr == MAP_FAILED) {
    LogMan::Msg::A("Failed to map memory region [0x%lx, 0x%lx) - %d", Offset, Size, Fixed);
    return nullptr;
  }
  LogMan::Msg::D("Mapped [0x%0lx, 0x%0lx) to 0x%0lx on host", Offset, Offset + Size, Ptr);

  return Ptr;
}

void *Memmap::FakeMapRegion(size_t Offset) {
  uintptr_t PtrOffset = reinterpret_cast<uintptr_t>(Base) + Offset;
  return reinterpret_cast<void*>(PtrOffset);
}

bool Memmap::AllocateSHMRegion(size_t Size) {
  const std::string SHMName = "VM" + std::to_string(getpid());
  SHMfd = shm_open(SHMName.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
  if (SHMfd == -1) {
    LogMan::Msg::E("Couldn't open SHM");
    return false;
  }

  // Unlink the shm file immediately to not leave it around
  shm_unlink(SHMName.c_str());

  // Extend the SHM to the size we requested
  if (ftruncate(SHMfd, Size) != 0) {
    LogMan::Msg::E("Couldn't set SHM size");
    return false;
  }

  SHMSize = Size;
  Base = MapRegion(0, SHMSize, false);

  return true;
}

void Memmap::DeallocateSHMRegion() {
  close(SHMfd);
}

namespace SU::VM {

  namespace KVM {
    struct MemoryRegion {
      uint64_t VirtualBase;
      uint64_t PhysicalBase;
      uint64_t RegionSize;
      void *MappedLocation;
    };

    // This only supports 4k pages
    class PML4Control final {
    public:
      // Set the virtual address that this lives in
      void SetVirtualAddress(uint64_t Address) { PML4VirtualAddress = Address; }
      void SetPhysicalAddress(uint64_t Address) { PML4PhysicalAddress = Address; }

      // Get the virtual address that the PML4 tables start at
      // They are mapped linearly in memory
      uint64_t GetVirtualAddress() const { return PML4VirtualAddress; }
      uint64_t GetPhysicalAddress() const { return PML4PhysicalAddress; }

      // Sets the pointer to the tables
      // This is the host pointer that is visible on both Host and Guest
      void SetTablePointer(uint64_t *Address) { Tables = Address; }

      // We need to calculate our PML4 table size
      // PTE = (MaxVirtualMemory / PAGE_SIZE) * Entry_Size
      // PDE = (MaxVirtualMemory >> 21) * Entry_Size
      // PDPE = (MaxVirtualMemory >> 30) * Entry_Size
      // PML4 = (MaxVirtualMemory >> 39) * Entry_Size
      uint64_t GetPML4Size() const {
        return (AlignUp(MaxVirtualMemory >> 39, 512) * // PML4
                512 * // PDPE
                512 + // PDE
                AlignUp(MaxVirtualMemory / PAGE_SIZE, 512)) * ENTRY_SIZE;
      }

      void AddMemoryMapping(uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size) {
        MemoryMappings.emplace_back(MemoryRegion{VirtualAddress, PhysicalAddress, Size});
      }

      uint64_t FindPhysicalInVirtual(uint64_t VirtualAddr) {
        for (auto Mapping : MemoryMappings) {
          if (VirtualAddr >= Mapping.VirtualBase &&
              VirtualAddr < (Mapping.VirtualBase + Mapping.RegionSize)) {
            return Mapping.PhysicalBase + (VirtualAddr - Mapping.VirtualBase);
          }
        }
        return ~0ULL;
      }

      uint64_t FindPhysicalFromTables(uint64_t VirtualAddr) {
        uint64_t *PML4_Table = (uint64_t*)((uintptr_t)Tables + 4096 * 0);
        uint64_t *PDPE_Table = (uint64_t*)((uintptr_t)Tables + 4096 * 1);
        uint64_t *PDE_Table  = (uint64_t*)((uintptr_t)Tables + 4096 * 2);
        uint64_t *PTE_Table  = (uint64_t*)((uintptr_t)Tables + 4096 * 3);

        uint64_t Physical_PML4_Table = PML4PhysicalAddress + 4096 * 0;
        uint64_t Physical_PDPE_Table = PML4PhysicalAddress + 4096 * 1;
        uint64_t Physical_PDE_Table  = PML4PhysicalAddress + 4096 * 2;
        uint64_t Physical_PTE_Table  = PML4PhysicalAddress + 4096 * 3;

        uint64_t PML4_Offset = (VirtualAddr >> 39) & 0x1FF;
        uint64_t PDPE_Offset = (VirtualAddr >> 30) & 0x1FF;
        uint64_t PDE_Offset = (VirtualAddr >> 21) & 0x1FF;
        uint64_t PTE_Offset = (VirtualAddr >> 12) & 0x1FF;
        uint64_t Physical_Offset = (VirtualAddr) & 0xFFF;
        return PTE_Table[PTE_Offset] & ~0xFFF;
      }

      void SetupTables();

    private:

      uint64_t GetPML4TableOffset(uint64_t Address) const {
        // There is only one PML4 table
        return 0;
      }

      uint64_t GetPML4EntryOffset(uint64_t Address) const {
        LogMan::Throw::A(Address < MaxVirtualMemory, "Address of 0x%lx is larger than our max virtual address size", Address);

        // First get the segment for the PML4
        uint64_t PML4Segment = (Address >> 39) & 0x1FF;

        // Each entry is 8 bytes
        // So the table entry is just the segment multiplied by the entry size
        return PML4Segment * ENTRY_SIZE;
      }

      uint64_t GetPDPEEntryOffset(uint64_t Address) const {
        LogMan::Throw::A(Address < MaxVirtualMemory, "Address of 0x%lx is larger than our max virtual address size", Address);

        // Get the segment for the PDPE
        uint64_t PDPESegment = (Address >> 30) & 0x1FF;

        // Each entry is 8 bytes
        // So the table entry is just the segment multiplied by the entry size
        return PDPESegment * ENTRY_SIZE;
      }

      uint64_t GetPDEEntryOffset(uint64_t Address) const {
        LogMan::Throw::A(Address < MaxVirtualMemory, "Address of 0x%lx is larger than our max virtual address size", Address);

        // Get the segment for the PDE
        uint64_t PDESegment = (Address >> 21) & 0x1FF;

        // Each entry is 8 bytes
        // So the table entry is just the segment multiplied by the entry size
        return PDESegment * ENTRY_SIZE;
      }

      uint64_t GetPTEEntryOffset(uint64_t Address) const {
        LogMan::Throw::A(Address < MaxVirtualMemory, "Address of 0x%lx is larger than our max virtual address size", Address);

        // Get the segment for the PTE
        uint64_t PTESegment = (Address >> 12) & 0x1FF;

        // Each entry is 8 bytes
        // So the table entry is just the segment multiplied by the entry size
        return PTESegment * ENTRY_SIZE;
      }

      uint64_t GetPDPETableOffset(uint64_t Address) const {
        uint64_t PDPETables = GetPDPEBase((uint64_t)0ULL);

        // We get the PML4 Entry number
        uint64_t PML4Entry = GetPML4EntryOffset(Address) / ENTRY_SIZE;

        // The PML4 entry number selects which PDE table we use
        return PDPETables + PML4Entry * 512;
      }

      uint64_t GetPDETableOffset(uint64_t Address) const {
        uint64_t PDETables = GetPDEBase((uint64_t)0ULL);

        // We get the PML4 Entry number
        uint64_t PML4Entry = GetPML4EntryOffset(Address) / ENTRY_SIZE;

        // We get the PDPE Entry number
        uint64_t PDPEEntry = GetPDPEEntryOffset(Address) / ENTRY_SIZE;

        return PDETables + PML4Entry * 512 + PDPEEntry * 512;
      }

      uint64_t GetPTETableOffset(uint64_t Address) const {
        uint64_t PTETables = GetPTEBase((uint64_t)0ULL);

        // We get the PML4 Entry number
        uint64_t PML4Entry = GetPML4EntryOffset(Address) / ENTRY_SIZE;

        // We get the PDPE Entry number
        uint64_t PDPEEntry = GetPDPEEntryOffset(Address) / ENTRY_SIZE;

        // We get the PDE Entry number
        uint64_t PDEEntry = GetPDEEntryOffset(Address) / ENTRY_SIZE;

        return PTETables + PML4Entry * 512 + PDPEEntry * 512 + PDEEntry * 512;
      }

      // PML4 base is just at our base pointer location
      template<typename T>
      T GetPML4Base(T Base) const {
        return Base;
      }

      uint64_t GetPML4TableEntryCount() const {
        return AlignUp(MaxVirtualMemory >> 39, 512);
      }

      template<typename T>
      T GetPDPEBase(T Base) const {
        uintptr_t Ptr = reinterpret_cast<uintptr_t>(Base);

        // PDPE is just past our PML4 table
        // PML4 is the top level table, which is only 1 table in size
        Ptr += GetPML4TableEntryCount() * ENTRY_SIZE;

        return reinterpret_cast<T>(Ptr);
      }

      // The PDPE table size is the number of PML4 entries
      // multiplied by the number of PDPE entries
      uint64_t GetPDPETableEntryCount() const {
        return GetPML4TableEntryCount() * 512;
      }

      template<typename T>
      T GetPDEBase(T Base) const {
        uintptr_t Ptr = reinterpret_cast<uintptr_t>(Base);

        // PML4 is the top level table, which is only 1 table in size
        // PDPE is just past our PML4 table
        // PDE is just past our PDPE table
        Ptr += GetPML4TableEntryCount() * ENTRY_SIZE +
               GetPDPETableEntryCount() * ENTRY_SIZE;

        return reinterpret_cast<T>(Ptr);
      }

      // The PDE table size is the number of PDPE entries
      // multiplied by the number of PDE entries
      uint64_t GetPDETableEntryCount() const {
        return GetPDPETableEntryCount() * 512;
      }

      template<typename T>
      T GetPTEBase(T Base) const {
        uintptr_t Ptr = reinterpret_cast<uintptr_t>(Base);

        // PML4 is the top level table, which is only 1 table in size
        // PDPE is just past our PML4 table
        // PDE is just past our PDPE table
        // PTE is just past our PDE table
        Ptr += GetPML4TableEntryCount() * ENTRY_SIZE +
               GetPDPETableEntryCount() * ENTRY_SIZE +
               GetPDETableEntryCount() * ENTRY_SIZE;

        return reinterpret_cast<T>(Ptr);
      }

      // The PTE table size is the maximum size of our virtual memory range
      // divided by page size, Aligned to the max table size
      uint64_t GetPTETableEntryCount() const {
        return AlignUp(MaxVirtualMemory / PAGE_SIZE, 512);
      }

      std::vector<MemoryRegion> MemoryMappings;
      uint64_t PML4VirtualAddress{};
      uint64_t PML4PhysicalAddress{};
      uint64_t *Tables;
      constexpr static uint64_t PAGE_SIZE = 4096;
      constexpr static uint64_t ENTRY_SIZE = 8;
      constexpr static uint64_t MaxVirtualMemory  = (1ULL << 36);
    };

    void PML4Control::SetupTables() {
      // Walk throw all of our provided mapping regions and set them up in the tables
      // We will always map virtual memory to physical memory linearly
      // Nothing special going on here

      uint64_t TotalPages{};

      for (auto &Region : MemoryMappings)
      {
        uint64_t VirtualAddress = Region.VirtualBase;
        uint64_t PhysicalAddress = Region.PhysicalBase;

        if (VirtualAddress & (PAGE_SIZE - 1)) {
          fprintf(stderr, "Memory Region virtual address [0x%lx, 0x%lx) needs to be page aligned\n", VirtualAddress, VirtualAddress + Region.RegionSize);
        }
        if (PhysicalAddress & (PAGE_SIZE - 1)) {
          fprintf(stderr, "Memory Region Physical address [0x%lx, 0x%lx) needs to be page aligned\n", PhysicalAddress, PhysicalAddress + Region.RegionSize);
        }
        if (Region.RegionSize & (PAGE_SIZE - 1)) {
          fprintf(stderr, "Memory region [0x%lx, 0x%lx) size 0x%lx is not page aligned\n", VirtualAddress, VirtualAddress + Region.RegionSize, Region.RegionSize);
        }
        fprintf(stderr, "[Virt] Memory [0x%lx, 0x%lx) mapped to physical address 0x%lx\n", VirtualAddress, VirtualAddress + Region.RegionSize, PhysicalAddress);

        uint64_t NumPages = Region.RegionSize / PAGE_SIZE;
        TotalPages += NumPages;

        // Now walk the tables for all the pages and map the table
        for (uint64_t i = 0; i < NumPages; ++i) {
          uint64_t OffsetVirtual = VirtualAddress + (i * PAGE_SIZE);
          uint64_t OffsetPhysical = PhysicalAddress + (i * PAGE_SIZE);

          uint64_t PML4_Table_Offset = GetPML4TableOffset(OffsetVirtual);
          uint64_t PDPE_Table_Offset = GetPDPETableOffset(OffsetVirtual);
          uint64_t PDE_Table_Offset = GetPDETableOffset(OffsetVirtual);
          uint64_t PTE_Table_Offset = GetPTETableOffset(OffsetVirtual);

          // These don't calculate which table they end up in
          // Just the offset in to the table
          uint64_t PML4_Entry_Offset = GetPML4EntryOffset(OffsetVirtual);
          uint64_t PDPE_Entry_Offset = GetPDPEEntryOffset(OffsetVirtual);
          uint64_t PDE_Entry_Offset = GetPDEEntryOffset(OffsetVirtual);
          uint64_t PTE_Entry_Offset = GetPTEEntryOffset(OffsetVirtual);

          uint64_t PML4_Entry = GetPML4EntryOffset(OffsetVirtual) / ENTRY_SIZE;
          uint64_t PDPE_Entry = GetPDPEEntryOffset(OffsetVirtual) / ENTRY_SIZE;
          uint64_t PDE_Entry = GetPDEEntryOffset(OffsetVirtual) / ENTRY_SIZE;
          uint64_t PTE_Entry = GetPTEEntryOffset(OffsetVirtual) / ENTRY_SIZE;

          uint64_t PDPE_Selected_Table = PML4PhysicalAddress + PDPE_Table_Offset;
          uint64_t PDE_Selected_Table = PML4PhysicalAddress + PDE_Table_Offset;
          uint64_t PTE_Selected_Table = PML4PhysicalAddress + PTE_Table_Offset;

          uint64_t *PML4_Table = (uint64_t*)((uintptr_t)Tables + PML4_Table_Offset);
          uint64_t *PDPE_Table = (uint64_t*)((uintptr_t)Tables + PDPE_Table_Offset);
          uint64_t *PDE_Table  = (uint64_t*)((uintptr_t)Tables + PDE_Table_Offset);
          uint64_t *PTE_Table  = (uint64_t*)((uintptr_t)Tables + PTE_Table_Offset);

          // Set the PML4_table to point to Base of the PDPE table offset
          PML4_Table[PML4_Entry] = PDPE_Selected_Table | 0b111;
          // Continue mapping
          PDPE_Table[PDPE_Entry] = PDE_Selected_Table | 0b0000'0000'0111;
          // Continue mapping
          PDE_Table[PDE_Entry]   = PTE_Selected_Table | 0b111;
          // This now maps directly to the region's physical address
          PTE_Table[PTE_Entry] = OffsetPhysical | 0b0001'00000011;
        }
      }
    }

    class KVMInstance final : public VMInstance {
      public:
        virtual ~KVMInstance();
        void SetPhysicalMemorySize(uint64_t Size) override { PhysicalUserMemorySize = Size; }
        void *GetPhysicalMemoryPointer() override { return VM_Addr; }

        void AddMemoryMapping(uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size) override {
          PML4.AddMemoryMapping(VirtualAddress, PhysicalAddress, Size);
        }

        void SetVMRIP(uint64_t VirtualAddress) override;
        bool Initialize() override;
        bool SetStepping(bool Step) override;
        bool Run() override;
        int ExitReason() override { return VCPUMemory->exit_reason; }
        uint64_t GetFailEntryReason() override { return VCPUMemory->fail_entry.hardware_entry_failure_reason; }
        VMInstance::RegisterState GetRegisterState() override;
        VMInstance::SpecialRegisterState GetSpecialRegisterState() override;

        void SetRegisterState(VMInstance::RegisterState &State) override;
        void SetSpecialRegisterState(VMInstance::SpecialRegisterState &State) override;
        void Debug() override;

      private:
        // FDs
        int KVM_FD{-1};
        int VM_FD{-1};
        int VCPU_FD{-1};

        constexpr static uint64_t VirtualBase = 0x0000'0001'0000'0000;
        // This is the size of the user's memory region
        uint64_t PhysicalUserMemorySize;

        // This is the actual size of the physical memory including additional structures that need to live in mem
        uint64_t PhysicalMemorySize;
        PML4Control PML4;

        kvm_run *VCPUMemory;
        uint32_t MemorySlot{};

        // Allocated memory
        void *PML4_Addr;
        void *VM_Addr;
        MemoryRegion IDTRegion {
          .RegionSize = 4096,
        };
        MemoryRegion GDTRegion {
          .RegionSize = 16 * 4096,
        };
        MemoryRegion APICRegion {
          .RegionSize = 4096,
        };
        bool SetupCPUInLongMode();
        bool SetupMemory();
        Memmap Mapper;
    };

    KVMInstance::~KVMInstance() {
      // Close all of our descriptors
      close(VCPU_FD);
      close(VM_FD);
      close(KVM_FD);

      // unmap all of our pointers
      munmap(VM_Addr, PhysicalMemorySize);
      munmap(PML4_Addr, PML4.GetPML4Size());
      munmap(IDTRegion.MappedLocation, IDTRegion.RegionSize);
    }

    bool KVMInstance::SetupMemory() {
      // ============================
      // Physical Memory map currently
      // ============================

      // Everything is mapped linearly from [0, end)
      // [UserMemory, UserMemory + Size)
      // [IDTRegion, IDTRegion + Size)
      // [GDTRegion, GDTRegion + Size)
      // [APICRegion, APICRegion + Size)
      // [PML4, PML4 + Size)
      //
      // ============================
      // Virtual Memory map currently
      // ============================
      // Virtual memory map directly mirrors the physical mapping

      uint64_t PhysicalMemoryBase = 0;
      // Calculate the amount of physical memory backing we need
      PhysicalMemorySize = PhysicalUserMemorySize;

      IDTRegion.PhysicalBase = PhysicalMemorySize;
      IDTRegion.VirtualBase = VirtualBase + IDTRegion.PhysicalBase; // Mapped to the same place
      PhysicalMemorySize += IDTRegion.RegionSize;

      GDTRegion.PhysicalBase = PhysicalMemorySize;
      GDTRegion.VirtualBase = VirtualBase + GDTRegion.PhysicalBase; // Mapped to the same place
      PhysicalMemorySize += GDTRegion.RegionSize;

      APICRegion.PhysicalBase = PhysicalMemorySize;
      APICRegion.VirtualBase = VirtualBase + APICRegion.PhysicalBase; // Mapped to the same place
      PhysicalMemorySize += APICRegion.RegionSize;

      // Physical and virtual map directly atm
      PML4.SetPhysicalAddress(PhysicalMemorySize);
      PML4.SetVirtualAddress(VirtualBase + PhysicalMemorySize);

      PhysicalMemorySize += PML4.GetPML4Size();

      // Align up just to make sure we are page aligned
      PhysicalMemorySize = AlignUp(PhysicalMemorySize, 4096);

      Mapper.AllocateSHMRegion(1ULL << 37);

      // Physical memory is just [0, PhysicalMemorySize)
      // Just map physical zero to virtual zero
      // This maps both our user set memory region and our PML4 region to be accessable
      PML4.AddMemoryMapping(VirtualBase, 0, PhysicalMemorySize);

      // Set up user memory
      {
        // Allocate the memory for the user physical memory
        VM_Addr = Mapper.MapRegion(0, PhysicalUserMemorySize, true);
        LogMan::Throw::A(VM_Addr != (void*)-1ULL, "Couldn't Allocate");

        // Map this memory to the guest VM
        kvm_userspace_memory_region Region = {
          .slot = MemorySlot++,
          .flags = 0,
          .guest_phys_addr = PhysicalMemoryBase,
          .memory_size = PhysicalUserMemorySize,
          .userspace_addr = (uint64_t)VM_Addr,
        };

        if (ioctl(VM_FD, KVM_SET_USER_MEMORY_REGION, &Region) == -1) {
          printf("Couldn't set guestPD region\n");
          return false;
        }

        // The PM4L setup with handle the page mapping
      }

      // Set up PML4
      {
        // Allocate the host facing PML4 memory
        PML4_Addr = Mapper.MapRegion(PML4.GetPhysicalAddress(), PML4.GetPML4Size(), true);
        LogMan::Throw::A(PML4_Addr != (void*)-1ULL, "Couldn't Allocate");

        if (PML4_Addr == reinterpret_cast<void*>(-1ULL)) {
          fprintf(stderr, "Couldn't allocate PML4 table memory");
          return false;
        }
        PML4.SetTablePointer(reinterpret_cast<uint64_t*>(PML4_Addr));

        // Map this address space in to the VM
        kvm_userspace_memory_region Region = {
          .slot = MemorySlot++,
          .flags = 0,
          .guest_phys_addr = PML4.GetPhysicalAddress(),
          .memory_size = PML4.GetPML4Size(),
          .userspace_addr = (uint64_t)PML4_Addr,
        };

        if (ioctl(VM_FD, KVM_SET_USER_MEMORY_REGION, &Region) == -1) {
          printf("Couldn't set guestPD region\n");
          return false;
        }

        // Setup the tables
        PML4.SetupTables();
      }

      // Set up GDT
      // GDT contains SEGMENTS
      {
        GDTRegion.MappedLocation = Mapper.MapRegion(GDTRegion.PhysicalBase, GDTRegion.RegionSize, true);
        LogMan::Throw::A(GDTRegion.MappedLocation != (void*)-1ULL, "Couldn't Allocate");

        // Map this memory to the guest VM
        kvm_userspace_memory_region Region = {
          .slot = MemorySlot++,
          .flags = 0,
          .guest_phys_addr = GDTRegion.PhysicalBase,
          .memory_size = GDTRegion.RegionSize,
          .userspace_addr = (uint64_t)GDTRegion.MappedLocation,
        };

        if (ioctl(VM_FD, KVM_SET_USER_MEMORY_REGION, &Region) == -1) {
          printf("Couldn't set guestPD region\n");
          return false;
        }

        // Set up the table entries
        union GDTSegment {
          struct NullSegment {
            uint32_t Unused_0;
            uint32_t Unused_1;
          } Null;
          struct CodeSegment {
             uint32_t Unused;
             unsigned Unused_0 : 8;
             unsigned Unused_1 : 1;
             unsigned Unused_2 : 1;
             unsigned C        : 1;
             unsigned MustBeSet: 2;
             unsigned DPL      : 2;
             unsigned P        : 1;
             unsigned Unused_3 : 4;
             unsigned Unused_4 : 1;
             unsigned L        : 1;
             unsigned D        : 1;
             unsigned Unused_5 : 1;
             unsigned Unused_6 : 8;
          } Code;
          struct DataSegment {
            uint32_t Unused_0;
            unsigned Unused_1 : 11;
            unsigned DataType : 2;
            unsigned Unused_2 : 2;
            unsigned P : 1;
            unsigned Unused_3 : 16;
          } Data;
        } __attribute__((packed));

        // Even though this is still a GDT segment
        // its size has increased to 64 bits which means it takes two slots
        struct SystemSegment {
          uint16_t SegmentLimitLow;
          uint16_t BaseLow;
          unsigned BaseLowMid       : 8;
          unsigned Type             : 4;
          unsigned MustBeZero_0     : 1;
          unsigned DPL              : 2;
          unsigned P                : 1;
          unsigned SegmentLimitHigh : 4;
          unsigned Unused_0         : 2;
          unsigned G                : 1;
          unsigned BaseLowHigh      : 8;
          uint32_t BaseHigh;
          unsigned Unused_1         : 8;
          unsigned MustBeZero_1     : 5;
          unsigned Unused_2         : 19;
        } __attribute__((packed));

        GDTSegment DefaultCodeSegment = {
          .Code = {
            .MustBeSet = 0b11,
          },
        };
        GDTSegment DefaultDataSegment = {
          .Data = {
            .DataType = 0b10,
          },
        };
        SystemSegment DefaultSystemSegment = {};

        unsigned Entries = GDTRegion.RegionSize / sizeof(GDTSegment);

        GDTSegment *GuestEntries = (GDTSegment*)GDTRegion.MappedLocation;

        // Null Segment
        GDTSegment NullSegment{};

        // Data segment
        GDTSegment DataSegment = DefaultDataSegment;
        DataSegment.Data.P = 1;

        // Code Segment
        GDTSegment CodeSegment = DefaultCodeSegment;
        CodeSegment.Code.P = 1;
        CodeSegment.Code.L = 1; // 64bit Code
        CodeSegment.Code.D = 0; // MUST be 0 in 64bit mode. CS.D = 0 and CS.L = 1

        // 64bit TSS segment
        SystemSegment Enable64BitTSS = DefaultSystemSegment;
        Enable64BitTSS.Type = 0b1001;
        Enable64BitTSS.P = 1;

        unsigned CurrentOffset = 0;

        // Null segment is first
        GuestEntries[CurrentOffset++] = NullSegment;
        GuestEntries[CurrentOffset++] = CodeSegment;
        GuestEntries[CurrentOffset++] = DataSegment;

        void *Ptr = &GuestEntries[CurrentOffset];
        CurrentOffset += 2;
        memcpy(Ptr, &Enable64BitTSS, sizeof(SystemSegment));
      }

      // Set up IDT
      // When an exception or interrupt occurs, the processor uses the interrupt vector number as an index in to the IDT
      // IDT is used in all operating modes
      // IDT contains GATES
      {
        IDTRegion.MappedLocation = Mapper.MapRegion(IDTRegion.PhysicalBase, IDTRegion.RegionSize, true);
        LogMan::Throw::A(IDTRegion.MappedLocation != (void*)-1ULL, "Couldn't Allocate");

        // Map this memory to the guest VM
        kvm_userspace_memory_region Region = {
          .slot = MemorySlot++,
          .flags = 0,
          .guest_phys_addr = IDTRegion.PhysicalBase,
          .memory_size = IDTRegion.RegionSize,
          .userspace_addr = (uint64_t)IDTRegion.MappedLocation,
        };

        if (ioctl(VM_FD, KVM_SET_USER_MEMORY_REGION, &Region) == -1) {
          printf("Couldn't set guestPD region\n");
          return false;
        }

        union GateDesc {
          struct CallGateType {
            uint16_t TargetLow;
            uint16_t TargetSelector;
            unsigned Unused_0     : 8;
            unsigned Type         : 4;
            unsigned MustBeZero   : 1;
            unsigned DPL          : 2;
            unsigned P            : 1;
            uint16_t TargetMid;
            uint32_t TargetHigh;
            uint32_t Pad;
          } Call;

          struct InterruptTrapType {
            uint16_t TargetLow;
            uint16_t TargetSelector;
            unsigned IST          : 2;
            unsigned Unused_0     : 6;
            unsigned Type         : 4;
            unsigned MustBeZero   : 1;
            unsigned DPL          : 2;
            unsigned P            : 1;
            uint16_t TargetMid;
            uint32_t TargetHigh;
            uint32_t Pad;
          } IntTrap;
        } __attribute__((packed));

        GateDesc Test;

        // Types 0x04-0x07 are illegal in 64bit mode
        uint8_t TypeCall = 0x0C;
        uint8_t TypeIDT  = 0x0E;
        uint8_t TypeTrap = 0x0F;
        Test.IntTrap.TargetLow = 0;
        Test.IntTrap.TargetSelector = 0;
        Test.IntTrap.P = 1; // If P == 0 then `Segment not present` fault occurs
        Test.IntTrap.IST = 0; // Must be zero otherwise we need a TSS register set up
        Test.IntTrap.Type = TypeTrap;
        Test.IntTrap.TargetMid = 0;
        Test.IntTrap.TargetHigh = 0;

        unsigned Entries = IDTRegion.RegionSize / sizeof(GateDesc);
        printf("%d IDT entries\n", Entries);
        GateDesc *GuestEntries = (GateDesc*)IDTRegion.MappedLocation;

        // Section 8.2 Vectors of the AMD64 system programmer's manual
        // Claims that the first 32 vectors are reserved for predefined exception and interrupt conditions
        // The IDT can fit 256 interrupts

        //Back to IDT
        for (unsigned i = 0; i < Entries; ++i) {
          GuestEntries[i] = Test;
        }
      }

      // Set up our initial APIC region
      {
        APICRegion.MappedLocation = Mapper.MapRegion(APICRegion.PhysicalBase, APICRegion.RegionSize, true);
        LogMan::Throw::A(APICRegion.MappedLocation != (void*)-1ULL, "Couldn't Allocate");

        // Map this memory to the guest VM
        kvm_userspace_memory_region Region = {
          .slot = MemorySlot++,
          .flags = 0,
          .guest_phys_addr = APICRegion.PhysicalBase,
          .memory_size = APICRegion.RegionSize,
          .userspace_addr = (uint64_t)APICRegion.MappedLocation,
        };

        if (ioctl(VM_FD, KVM_SET_USER_MEMORY_REGION, &Region) == -1) {
          printf("Couldn't set guestPD region\n");
          return false;
        }

        uint64_t *APICRegisters = (uint64_t*)APICRegion.MappedLocation;
        for (unsigned i = 0; i < 0x54; ++i) {
          APICRegisters[i] = ~0ULL;
        }
      }

      return true;
    }

    bool KVMInstance::SetupCPUInLongMode() {
      kvm_sregs SpecialRegisters = {
        .cs = {
          .base = 0,   // Ignored in 64bit mode
          .limit = 0U, // Ignored in 64bit mode
          .present = 1,
          .dpl = 0,    // Data-pivilege-level Still used in 64bit mode to allow accesses 
          .db = 0,     // Ignored in 64bit mode
          .l = 1,      // 1 for 64bit execution mode. 0 if you want 32bit compatibility
          .g = 0,      // Ignored in 64bit mode
        },
        .ds = {
          .present = 1,
        },
        .es = {
          .present = 1,
        },
        .fs = {
          .present = 1,
        },
        .gs = {
          .present = 1,
        },
        .ss = {
          .present = 1,
        },
        // Do we need the LDT(Local Descriptor Table) and TR (Task Register)?
        .tr = {
          .base = 0,
          .limit = 0,
        },

        .ldt = {
          .base = 0,
          .limit = 0,
        },
        .gdt = {
          .base = GDTRegion.VirtualBase,
          .limit = (uint16_t)GDTRegion.RegionSize,
        },
        .idt = {
          .base = IDTRegion.VirtualBase,
          .limit = (uint16_t)IDTRegion.RegionSize,
        },

        .cr0 = (1U << 0) | // Protected Mode enable. Required for 64bit mode
//          (1U << 1) | // Monitor coprocessor
          (0U << 2) |  // EM bit. 0 means State x87 FPU is present
          (0U << 3) | // TS bit. If set causes a device not available exception
          (1U << 31) | // (PG) Enable paging bit. Required for 64bit mode
          0,
        .cr3 = PML4.GetPhysicalAddress() | // PDBR in PAE mode. Required for 64bit mode
//          (1U << 4) | // Page cache disable
          0,
        .cr4 = (1U << 5) | // PAE. Required for 64bit mode
//          (1U << 0) | // (VME) VM86 extensions
//          (1U << 1) | // (PVI) Virtual interrupt flags
//          (1U << 3) | // (DE) Debug extensions
//          (1U << 6) | // (MCE) Machine check enable
//          (1U << 7) | // (PGE) Global page enable
          (1U << 9) | // OSFXSR. Required to enable SSE
          // (1U << 18) | // XXX: OSXSave. Required to enable AVX. Requires setting the guest CPUID to say it supports xsave
          0,
        .efer = (1U << 10) | // (LMA) Long Mode Active.
          (1U << 8) | // (LME) Long Mode Enable
          0,
        .apic_base = (1 << 11) | // (AE) APIC Enable
          // (APICRegion.PhysicalBase >> 12 << 32), // ABA, APIC Base Address. ABA is zext by 12 bits to form a 52bit physical address
          0,
      };

      kvm_regs Registers = {
        .rsp = 0xFF0, 
        .rip = ~0ULL,
        .rflags = 0x2 | // 0x2 is the default state of rflags
          (1U << 9) | // By default enable hardware interrupts
          (1U << 19) | // By default enable virtual hardware interrupts
          (1U << 21) | // CPU ID detection
          0,
      };

      if (ioctl(VCPU_FD, KVM_SET_SREGS, &SpecialRegisters) == -1) {
        fprintf(stderr, "Couldn't set kvm_sregs\n");
        return false;
      }

      if (ioctl(VCPU_FD, KVM_SET_REGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't set kvm_regs\n");
        return false;
      }
      return true;
    }

    bool KVMInstance::Initialize() {
      int APIVersion;
      int VCPUSize;
      int Result;

      // Open the KVM FD
      KVM_FD = open("/dev/kvm", O_RDWR | O_CLOEXEC);
      if (KVM_FD == -1) {
        fprintf(stderr, "Couldn't open /dev/kvm: %s\n", strerror(errno));
        goto err;
      }

      // Check the KVM version to make sure we support it
      APIVersion = ioctl(KVM_FD, KVM_GET_API_VERSION, 0);
      if (APIVersion == -1) {
        fprintf(stderr, "Couldn't get KVM Version\n");
        goto err;
      }

      if (APIVersion != 12) {
        fprintf(stderr, "Unknown KVM version: %d\n", APIVersion);
        goto err;
      }

      // Create the VM FD
      VM_FD = ioctl(KVM_FD, KVM_CREATE_VM, 0);
      if (VM_FD == -1) {
        fprintf(stderr, "Couldn't create VM FD\n");
        goto err;
      }

      // Create the virtual CPU core
      VCPU_FD = ioctl(VM_FD, KVM_CREATE_VCPU, 0);
      if (VCPU_FD == -1) {
        fprintf(stderr, "Couldn't open VM's VCPU");
        goto err;
      }

      // Get the VCPU's mmap size
      VCPUSize = ioctl(KVM_FD, KVM_GET_VCPU_MMAP_SIZE, 0);
      if (VCPUSize == -1 || VCPUSize < sizeof(kvm_run)) {
        fprintf(stderr, "Received unexpected kvm_run size: %d\n", VCPUSize);
        goto err;
      }

      // Map in the kvm_run structure that is mapped to VCPU_FD at offset 0
      VCPUMemory = reinterpret_cast<kvm_run*>(mmap(nullptr, VCPUSize, PROT_READ | PROT_WRITE, MAP_SHARED, VCPU_FD, 0));
      LogMan::Throw::A(VCPUMemory != (void*)-1ULL, "Couldn't Allocate");

      if (!SetupMemory()) {
        fprintf(stderr, "Couldn't set up the CPU's memory\n");
        goto err;
      }

      if (!SetupCPUInLongMode()) {
        fprintf(stderr, "Couldn't initialize CPU in Long Mode\n");
        goto err;
      }

      // Set a default TSS
      // AMD SVM literally doesn't care but Intel VMX mandates it
      // Needs to fit under 4GB and also have a minimum of three pages
      Result = ioctl(VM_FD, KVM_SET_TSS_ADDR, -1U - (4096 * 3));
      if (Result == -1) {
        fprintf(stderr, "Couldn't set TSS ADDR\n");
        goto err;
      }

      return true;
err:
      if (VCPU_FD != -1) {
        close(VCPU_FD);
      }
      if (VM_FD != -1) {
        close(VM_FD);
      }
      if (KVM_FD != -1) {
        close(KVM_FD);
      }
      return false;
    }

    void KVMInstance::SetVMRIP(uint64_t VirtualAddress) {
      kvm_regs Registers;
      if (ioctl(VCPU_FD, KVM_GET_REGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't get current set registers\n");
        return;
      }

      // Set RIP
      Registers.rip = VirtualAddress;
      if (ioctl(VCPU_FD, KVM_SET_REGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't set kvm_regs\n");
        return;
      }
    }

    bool KVMInstance::SetStepping(bool Step) {
      kvm_guest_debug GuestDebug{};

      if (Step) {
        GuestDebug.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP;
      }
      else {
        GuestDebug.control = 0;
      }

      int Result = ioctl(VCPU_FD, KVM_SET_GUEST_DEBUG, &GuestDebug);
      if (Result == -1) {
        fprintf(stderr, "Couldn't set single step debugging\n");
        return false;
      }
      return true;
    }

    bool KVMInstance::Run() {
      // Kick off our virtual CPU
      // This ioctl will block until the VM exits
      // If single stepping it will return after the CPU has stepping a single instruction
      int Result = ioctl(VCPU_FD, KVM_RUN, 0);
      if (Result == -1) {
        fprintf(stderr, "KVM_RUN failed\n");
        return false;
      }
      kvm_regs Registers;
      ioctl(VCPU_FD, KVM_GET_REGS, &Registers);
      printf("RIP is now: 0x%llx\n", Registers.rip);

      return true;
    }

    VMInstance::RegisterState KVMInstance::GetRegisterState() {
      kvm_regs Registers;
      if (ioctl(VCPU_FD, KVM_GET_REGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't get KVM register state\n");
        return {};
      }
      VMInstance::RegisterState State;
      // This map 1:1
      memcpy(&State, &Registers, sizeof(kvm_regs));
      return State;
    }

    VMInstance::SpecialRegisterState KVMInstance::GetSpecialRegisterState() {
      kvm_sregs Registers;
      if (ioctl(VCPU_FD, KVM_GET_SREGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't get KVM special register state\n");
        return {};
      }
      VMInstance::SpecialRegisterState State;
      State.fs.base = Registers.fs.base;
      State.gs.base = Registers.gs.base;
      return State;
    }

    void KVMInstance::SetRegisterState(VMInstance::RegisterState &State) {
      kvm_regs Registers;
      // This map 1:1
      static_assert(offsetof(kvm_regs, rax) == offsetof(VMInstance::RegisterState, rax));
      static_assert(offsetof(kvm_regs, rbx) == offsetof(VMInstance::RegisterState, rbx));
      static_assert(offsetof(kvm_regs, rcx) == offsetof(VMInstance::RegisterState, rcx));
      static_assert(offsetof(kvm_regs, rdx) == offsetof(VMInstance::RegisterState, rdx));
      static_assert(offsetof(kvm_regs, rsi) == offsetof(VMInstance::RegisterState, rsi));
      static_assert(offsetof(kvm_regs, rdi) == offsetof(VMInstance::RegisterState, rdi));
      static_assert(offsetof(kvm_regs, rsp) == offsetof(VMInstance::RegisterState, rsp));
      static_assert(offsetof(kvm_regs, rbp) == offsetof(VMInstance::RegisterState, rbp));
      static_assert(offsetof(kvm_regs, r8) == offsetof(VMInstance::RegisterState, r8));
      static_assert(offsetof(kvm_regs, r9) == offsetof(VMInstance::RegisterState, r9));
      static_assert(offsetof(kvm_regs, r10) == offsetof(VMInstance::RegisterState, r10));
      static_assert(offsetof(kvm_regs, r11) == offsetof(VMInstance::RegisterState, r11));
      static_assert(offsetof(kvm_regs, r12) == offsetof(VMInstance::RegisterState, r12));
      static_assert(offsetof(kvm_regs, r13) == offsetof(VMInstance::RegisterState, r13));
      static_assert(offsetof(kvm_regs, r14) == offsetof(VMInstance::RegisterState, r14));
      static_assert(offsetof(kvm_regs, r15) == offsetof(VMInstance::RegisterState, r15));
      static_assert(offsetof(kvm_regs, rip) == offsetof(VMInstance::RegisterState, rip));
      static_assert(offsetof(kvm_regs, rflags) == offsetof(VMInstance::RegisterState, rflags));

      memcpy(&Registers, &State, sizeof(kvm_regs));

      if (ioctl(VCPU_FD, KVM_SET_REGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't set kvm_regs\n");
        return;
      }
    }

    void KVMInstance::SetSpecialRegisterState(VMInstance::SpecialRegisterState &State) {
      kvm_sregs Registers;

      // First read the state so we don't destroy a bunch of data
      if (ioctl(VCPU_FD, KVM_GET_SREGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't get KVM special register state\n");
        return;
      }
      Registers.fs.base = State.fs.base;
      Registers.gs.base = State.gs.base;

      if (ioctl(VCPU_FD, KVM_SET_SREGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't set kvm_sregs\n");
        return;
      }
    }

    void KVMInstance::Debug() {
      kvm_sregs Registers;
      if (ioctl(VCPU_FD, KVM_GET_SREGS, &Registers) == -1) {
        fprintf(stderr, "Couldn't get KVM special register state\n");
        return;
      }

      auto PrintDTable = [](const char *Name, kvm_dtable &Table) {
        LogMan::Msg::D("%s 0x%016llx 0x%04x", Name, Table.base, Table.limit);
      };

      auto PrintSegment = [](const char *Name, kvm_segment &Segment) {
        LogMan::Msg::D("%s 0x%016llx 0x%08x 0x%04x 0x%02x %d   %d  %d %d %d %d   %d",
            Name, Segment.base, Segment.limit, Segment.selector, Segment.type,
            Segment.present, Segment.dpl, Segment.db, Segment.s, Segment.l, Segment.g, Segment.avl);
      };

      SU::VM::VMInstance::RegisterState State;
      State = GetRegisterState();
      LogMan::Msg::D("================");
      LogMan::Msg::D("             RAX              RBX              RCX              RDX");
      LogMan::Msg::D("%016lx %016lx %016lx %016lx", State.rax, State.rbx, State.rcx, State.rdx);
      LogMan::Msg::D("             RSI              RDI              RSP              RBP");
      LogMan::Msg::D("%016lx %016lx %016lx %016lx", State.rsi, State.rdi, State.rsp, State.rbp);
      LogMan::Msg::D("              R8               R9              R10              R11");
      LogMan::Msg::D("%016lx %016lx %016lx %016lx", State.r8, State.r9, State.r10, State.r11);
      LogMan::Msg::D("             R12              R13              R14              R15");
      LogMan::Msg::D("%016lx %016lx %016lx %016lx", State.r12, State.r13, State.r14, State.r15);
      LogMan::Msg::D("             RIP           RFLAGS");
      LogMan::Msg::D("%016lx %016lx", State.rip, State.rflags);

      LogMan::Msg::D("================");
      LogMan::Msg::D("cr0:             cr2:             cr3:");
      LogMan::Msg::D("%016llx %016llx %016llx", Registers.cr0, Registers.cr2, Registers.cr3);
      LogMan::Msg::D("cr4:             cr6:");
      LogMan::Msg::D("%016llx %016llx", Registers.cr4, Registers.cr8);

      LogMan::Msg::D("================");
      LogMan::Msg::D("Reg Base               Limit      Select Type p dpl db s l g avl");

      PrintSegment("CS ", Registers.cs);
      PrintSegment("DS ", Registers.ds);
      PrintSegment("ES ", Registers.es);
      PrintSegment("FS ", Registers.fs);
      PrintSegment("GS ", Registers.gs);
      PrintSegment("SS ", Registers.ss);

      PrintSegment("TR ", Registers.tr);
      PrintSegment("LDT", Registers.ldt);

      LogMan::Msg::D("================");
      PrintDTable("GDT", Registers.gdt);
      PrintDTable("IDT", Registers.idt);

      LogMan::Msg::D("================");
      LogMan::Msg::D("EFER: 0x%016llx APIC: 0x%016llx", Registers.efer, Registers.apic_base);
      LogMan::Msg::D("================");
      LogMan::Msg::D("Interrupt Bitmap");
      for (int i = 0; i < (KVM_NR_INTERRUPTS + 63) / 64; ++i) {
        LogMan::Msg::D("Interrupt: %d: 0x%016llx", Registers.interrupt_bitmap[i]);
      }
    }
  }

  VMInstance* VMInstance::Create() {
    return new KVM::KVMInstance{};
  }
}
