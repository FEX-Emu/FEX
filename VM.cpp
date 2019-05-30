#include "VM.h"

#include <memory>
#include <fcntl.h>
#include <linux/kvm.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>

namespace SU::VM {

  namespace KVM {
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

      // 4k page mode uses 4 4096 byte tables
      // Each table has entries that are 8bytes in size. So each table is 512 elements in size
      // Different page modes drop the number of pages
      uint64_t GetPML4Size() const { return 4096 * 4; }

      void AddMemoryMapping(uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size) {
        MemoryMappings.emplace_back(MemoryRegion{VirtualAddress, PhysicalAddress, Size});
      }

      void SetupTables();
    private:
      struct MemoryRegion {
        uint64_t VirtualBase;
        uint64_t PhysicalBase;
        uint64_t RegionSize;
      };
      std::vector<MemoryRegion> MemoryMappings;
      uint64_t PML4VirtualAddress{};
      uint64_t PML4PhysicalAddress{};
      uint64_t *Tables;
      constexpr static uint64_t PAGE_SIZE = 4096;
    };

    void PML4Control::SetupTables() {
      // Walk throw all of our provided mapping regions and set them up in the tables
      // We will always map virtual memory to physical memory linearly
      // Nothing special going on here

      // These are all allocated linearly
      uint64_t *PML4_Table = (uint64_t*)((uintptr_t)Tables + 4096 * 0);
      uint64_t *PDPE_Table = (uint64_t*)((uintptr_t)Tables + 4096 * 1);
      uint64_t *PDE_Table  = (uint64_t*)((uintptr_t)Tables + 4096 * 2);
      uint64_t *PTE_Table  = (uint64_t*)((uintptr_t)Tables + 4096 * 3);

      uint64_t Physical_PML4_Table = PML4PhysicalAddress + 4096 * 0;
      uint64_t Physical_PDPE_Table = PML4PhysicalAddress + 4096 * 1;
      uint64_t Physical_PDE_Table  = PML4PhysicalAddress + 4096 * 2;
      uint64_t Physical_PTE_Table  = PML4PhysicalAddress + 4096 * 3;

      for (auto &Region : MemoryMappings) {
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

        uint64_t NumPages = Region.RegionSize / PAGE_SIZE;

        uint64_t PML4_Offset = (VirtualAddress >> 39) & 0x1FF;
        uint64_t PDPE_Offset = (VirtualAddress >> 30) & 0x1FF;
        uint64_t PDE_Offset = (VirtualAddress >> 21) & 0x1FF;
        uint64_t PTE_Offset = (VirtualAddress >> 12) & 0x1FF;
        uint64_t Physical_Offset = (VirtualAddress ) & 0xFFF;

        // Now walk the tables for all the pages and map the table
        for (uint64_t i = 0; i < NumPages; ++i) {
          // Set the PML4_table to point to Base of the PDPE table offset
          PML4_Table[PML4_Offset] = (uint64_t)Physical_PDPE_Table | 0b11;
          // Continue mapping
          PDPE_Table[PDPE_Offset] = (uint64_t)Physical_PDE_Table | 0b11;
          // Continue mapping
          PDE_Table[PDE_Offset]   = (uint64_t)Physical_PTE_Table | 0b11;
          // This now maps directly to the region's physical address
          PTE_Table[PTE_Offset] = PhysicalAddress | 0b11;
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

      private:
        // FDs
        int KVM_FD{-1};
        int VM_FD{-1};
        int VCPU_FD{-1};

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
        bool SetupCPUInLongMode();
        bool SetupMemory();
    };

    KVMInstance::~KVMInstance() {
      // Close all of our descriptors
      close(VCPU_FD);
      close(VM_FD);
      close(KVM_FD);

      // unmap all of our pointers
      munmap(VM_Addr, PhysicalMemorySize);
      munmap(PML4_Addr, PML4.GetPML4Size());
    }

    bool KVMInstance::SetupMemory() {
      uint64_t PhysicalMemoryBase = 0;
      // Calculate the amount of physical memory backing we need
      PhysicalMemorySize = PhysicalUserMemorySize;

      // Physical and virtual map directly atm
      PML4.SetPhysicalAddress(PhysicalMemorySize);
      PML4.SetVirtualAddress(PhysicalMemorySize);

      PhysicalMemorySize += PML4.GetPML4Size();
      // Physical memory is just [0, PhysicalMemorySize)
      // Just map physical zero to virtual zero
      // This maps both our user set memory region and our PML4 region to be accessable
      PML4.AddMemoryMapping(0, 0, PhysicalMemorySize);

      // Set up PML4
      {
        // Allocate the host facing PML4 memory
        PML4_Addr = mmap(nullptr, PML4.GetPML4Size(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        if (PML4_Addr == reinterpret_cast<void*>(-1ULL)) {
          fprintf(stderr, "Couldn't allocate PML4 table memory");
          return false;
        }
        PML4.SetTablePointer(reinterpret_cast<uint64_t*>(PML4_Addr));

        // Map this address space in to the VM
        kvm_userspace_memory_region Region = {
          .slot = MemorySlot++,
          .flags = 0, // Read-only?
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

      // Set up user memory
      {
        // Allocate the memory for the user physical memory
        VM_Addr = mmap(nullptr, PhysicalUserMemorySize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

        // Map this memory to the guest VM
        kvm_userspace_memory_region Region = {
          .slot = MemorySlot++,
          .flags = 0, // Read-only?
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

      return true;
    }

    bool KVMInstance::SetupCPUInLongMode() {
      kvm_sregs SpecialRegisters = {
        .cs = {
          .base = 0,   // Ignored in 64bit mode
          .limit = 0U, // Ignored in 64bit mode
          .g = 0,      // Ignored in 64bit mode
          .db = 0,     // Ignored in 64bit mode
          .dpl = 0,    // Data-pivilege-level Still used in 64bit mode to allow accesses
          .l = 1,      // 1 for 64bit execution mode. 0 if you want 32bit compatibility
        },
        .cr0 = (1U << 0) | // Protected Mode enable. Required for 64bit mode
          (1U << 31) | // (PG) Enable paging bit. Required for 64bit mode
          0,
        .cr3 = PML4.GetVirtualAddress(), // PDBR in PAE mode. Required for 64bit mode
        .cr4 = (1 << 5) | // PAE. Required for 64bit mode
          0,
        .efer = (1 << 10) | // Long Mode Active
          (1 << 8) | // Long Mode Enable
          0,
        .gdt = { // Is GDT actually necessary?
          .base = 0,
          .limit = 0,
        }
      };

      kvm_regs Registers = {
        .rip = ~0ULL, // Make sure the user sets the RIP
        .rflags = 0x2, // 0x2 is the default state of rflags
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

      if (!SetupMemory()) {
        fprintf(stderr, "Couldn't set up the CPU's memory\n");
        goto err;
      }

      if (!SetupCPUInLongMode()) {
        fprintf(stderr, "Couldn't initialize CPU in Long Mode\n");
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
      if (ioctl(VCPU_FD, KVM_GET_REGS, &Registers) == -1) {
        printf("Couldn't set registers\n");
      }
      printf("RIP ended up at 0x%llx\n", Registers.rip);

      printf("EAX: 0x%llx\n", Registers.rax);

      return true;
    }
  }

  VMInstance* VMInstance::Create() {
    return new KVM::KVMInstance{};
  }
}
