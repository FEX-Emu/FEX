#pragma once

#include <cstdint>
#include <memory>

namespace FEX::HLE {
  constexpr uint32_t X86_64_MAP_32BIT = 0x40;

  class MemAllocator {
  public:
    virtual ~MemAllocator() = default;
    virtual void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = 0;
    virtual int munmap(void *addr, size_t length) = 0;
    virtual void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) = 0;
    // Global symbol collision on undercase
    virtual uint64_t Shmat(int shmid, const void* shmaddr, int shmflg, uint32_t *ResultAddress) = 0;
    virtual uint64_t Shmdt(const void* shmaddr) = 0;
  };

  std::unique_ptr<FEX::HLE::MemAllocator> Create32BitAllocator();
  std::unique_ptr<FEX::HLE::MemAllocator> CreatePassthroughAllocator();
}
