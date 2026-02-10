# Dual allocator usage
FEX-Emu uses two different heap allocators at once, each for different purposes:
- rpmalloc: The primary heap allocator (to keep FEX's internal allocations out of the 32-bit address space used by guest applications)
- jemalloc_glibc: The second heap allocator (to add allocation introspection features used by thunks)

## rpmalloc - primary heap allocator
This allocator overrides `mmap` and `munmap` by forwarding them to FEXCore's internal VMA region allocator.

All of FEXCore's `fextl::` namespaced objects allocate memory with this method.

### FEXCore internal VMA region allocator
When running a 32-bit guest application, the VMA region allocator allocates from memory *above* the 4GB of virtual address space reserved for the
32-bit application.

This ensures that all of FEX's allocations stay out of the lower 32-bit 4GB VA space, since games would quickly run out of virtual address space
otherwise.

When running a 64-bit guest application, this VMA region allocator is disabled and passes through to mmap and munmap in the host kernel.

## jemalloc_glibc - secondary heap allocator
This heap allocator replaces the host glibc's allocator using weak symbol overriding. It adds introspection features used for thunking, but has no
functional differences otherwise: All memory is allocated in the 4GB of 32-bit address space. The FEXCore VMA region allocator is explicitly **not**
involved hence.

All native shared libraries use this allocator including the host-side of thunks.

Internally, all allocations that go through this heap allocator use the kernel mmap and munmap interface.

### Thunks
Thunks may allocate memory either through the guest-side (on the guest glibc heap) or the host-side (on the `jemalloc_glibc` heap). To properly free
this memory, FEX must be able to determine which heap allocator it belongs to.

`glibc` provides no public interface to do this, but FEX's `jemalloc` fork does. The `is_known_allocation` function is used by FEXCore to query
whether a given pointer originated from the `jemalloc_glibc` allocator. This enables FEXCore to determine the appropriate heap for freeing the
pointer.
