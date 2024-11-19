# Memory allocation routines
## What is the problem?
FEX-Emu needs to allocate memory differently than regular applications. This problem happens because FEX runs both 32-bit and 64-bit guest
applications in the same address space as FEX itself. When running 32-bit applications, FEX reserves up all memory above 4GB in order to correctly
emulate the 32-bit address space. We then use that reserved space for FEX allocations, so we don't interrupt application's own allocations.

### Why not just replace the system allocator?
We could control the placement of FEX's internal allocations by overriding the system
allocator. However, 32-bit thunks (and their corresponding native host libraries) still
need to allocate memory in the lower 4 GB of memory so that they produce
guest-accessible pointers. Since overriding the system allocator is a global operation,
selectively overriding it like this is not possible.

Since we found no way to resolve this conflict, we had to resort to the alternative of avoiding use of the system allocator for FEX's internal
allocations entirely (where possible).

## Sub-projects and applications that need to follow this.
- FEXCore
- FEXInterpreter/FEXLoader

## Sub-projects that explicitly cannot follow this
- Thunks

## APIs which allocate memory that FEX needs to avoid
Most C++ APIs allow you to replace their allocators, but some don't and we need to avoid those APIs. If FEX uses them then the 32-bit application
running might run out of memory. This isn't an all encompassing list and we will add to it as our CI captures more problems.

### `get_nprocs_conf`
Use `FEXCore::CPUInfo::CalculateNumberOfCPUs` instead.

### `getcwd`
Don't use getcwd with a nullptr buffer. It will allocate memory behind our back and return a pointer that needs a free.

### `strerror`
This allocates and frees memory based on locale! Even with C local it'll attempt to free(0).
FEX-Emu should avoid using this function and instead just return the number.
If necessary FEX will provide its own routine for getting this string back.

### `std::make_unique`
Use `fextl::make_unique` instead.

### `std::unique_ptr`
Use `fextl::unique_ptr` instead.

### `std::filesystem`
This namespace is /highly/ likely to allocate memory behind our back.
FEX should avoid using this API as much as possible.

#### std::filesystem::path
#### std::filesystem::path::is_relative
Use `FHU::Filesystem::IsRelative` instead.

#### std::filesystem::absolute
Always allocates memory.
Use `realpath` instead.

#### std::filesystem::exists
Creates a std::filesystem::path when passing in to it.
Use `FHU::Filesystem::Exists` instead.

#### std::filesystem::canonical
Always allocates memory.
Use `realpath` instead.

#### std::filesystem::path::lexically_normal
Use `FHU::Filesystem::LexicallyNormal` instead.

#### std::filesystem::create_directory
#### std::filesystem::create_directories
Creates a std::filesystem::path when passing in to it.
Use `FHU::Filesystem::CreateDirectory` and `FHU::Filesystem::CreateDirectories` instead.

#### std::filesystem::path::parent_path
Use `FHU::Filesystem::ParentPath` instead.

#### std::filesystem::path::filename
Use `FHU::Filesystem::GetFilename` instead.

#### std::filesystem::copy_file
Use `FHU::Filesystem::CopyFile` instead.

#### std::filesystem::temp_directory_path
See `GetTempFolder()` in `FEXServerClient.cpp` (split/move to `FHU::Filesystem` if needed by other users).

### `std::fstream`
This API always allocates memory and should be avoided.
Use a combination of open and fextl::string APIs instead of fstream.

### `std::fwrite`
This API allocates a buffer in the background for buffering output.
Use raw `write` instead.

### `std::string`
Use `fextl::string` instead.

#### std::to_string
Use `fextl::fmt::format` instead.

### `std::stol`
### `std::stoul`
### `std::stoll`
### `std::stoull`
These all consume a `std::string` as their first argument. Use the equivalent functions that don't use `std::string`
- `std::strtol`
- `std::strtoul`
- `std::strtoll`
- `std::strtoull`

### `fmt::`
### `fmt::format`
Use `fextl::fmt::` instead

### APIs that FEX doesn't have a replacement for
Don't use any of these APIs in FEXLoader/FEXInterpreter. Shoutout to
[this](https://stackoverflow.com/questions/43056338/standard-library-facilities-which-allocate-but-dont-use-an-allocator) StackOverflow post for this
huge list.

#### `std::any`
#### `std::function` and lambdas
One must take additional considerations when using these to ensure that they don't allocate memory. These don't have any way to replace which
allocator is used for these objects. Additionally there is no way up-front to know if these will allocate memory or if the compiler will use
small-function optimizations to avoid allocations. The only real way to check this is to enable the glibc faulting compile option.
- Lambdas without anything in the capture list will never allocate memory.
- One pointer in the capture list is likely to hit small-function optimizations and not allocate memory.
   - This isn't guaranteed.
- Passing the `std::function` as an argument is unlikely to optimize away their memory allocations.

#### `std::valarray`
#### `std::filebuf`
#### `std::inplace_merge`
#### `<stdexcept>`
#### `std::boyer_moore_searcher`
#### `std::filesystem::path`
#### `std::filesystem::directory_iterator`
#### `std::regex`
#### `std::thread`
#### `std::async`
#### `std::packaged_task`
#### `std::promise`
#### `<iostream>`
#### Remember this is not an all-encompassing list! We may find APIs that still allocate memory and need to be avoided!

### Regular memory allocation routines.
Don't use these directly as they will the glibc allocator.

#### mmap
Use `FEXCore::Allocator::mmap`

#### munmap
Use `FEXCore::Allocator::munmap`

#### malloc
Use `FEXCore::Allocator::malloc`

#### calloc
Use `FEXCore::Allocator::calloc`

#### memalign 
Use `FEXCore::Allocator::memalign`

#### valloc
Use `FEXCore::Allocator::valloc`

#### posix_memalign
Use `FEXCore::Allocator::posix_memalign`

#### realloc
Use `FEXCore::Allocator::realloc`

#### free
Use `FEXCore::Allocator::free`

#### aligned_alloc
Use `FEXCore::Allocator::aligned_alloc`

#### __libc_malloc
#### __libc_calloc
#### __libc_memalign
#### __libc_valloc
#### __posix_memalign
#### __malloc_usable_size
!! DO NOT USE !!

## How does FEX ensure that this paradigm doesn't break?
FEX has the cmake option `ENABLE_GLIBC_ALLOCATOR_HOOK_FAULT` to hook in to glibc's allocator and fault if anything is allocating through it.
This can't be used with thunks for thunk testing as those actually use the glibc allocator.
CI will run FEX's test suite with extra verification to ensure FEX makes no allocations are made through glibc. Thunking must be disabled for this
run, since thunks are by design the only place where glibc allocation still happen.
