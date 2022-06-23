
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <tuple>
#include <unordered_map>
#include <shared_mutex>

#include <sys/mman.h>

// Copyable functions

// Public API
//
// assuming typedef void some_fn_type(int, int);
// DECL_COPYABLE_TRAMPLOLINE(some_fn_type) will generate the trampolines, static structures, canonical fn, etc
// then
// some_fn_type *binder::make_instance(some_fn_type *target, marshaler *marshaler) can be used to make a new bound function
// some_fn_type *binder::get_target(some_fn_type *bound_fn) can be used to lookup the target function of a bound function


// Internals
//
// binder::inner<type>::canonical has a special end-of-function marker using asm goto to resurrect dead code
// binder::inner<type>::guest_to_host and binder::inner<type>::host_to_guest hold mappings. These are per type
//  as functions might be called with different parameters
// binder::inner<type>::mutex protects guest_to_host, host_to_guest

#define copyable_logf(...)

class binder {
    using offsets_t = int;
    
    struct alignas(16) instance_info_t {
        void *target;
        void *marshaler;
    };

    template<typename R, typename... Args>
    struct inner;

    template<typename R, typename... Args>
    struct inner<R(Args...)> {
        using target_t = R(Args...);
        using marshaler_t = R(Args..., target_t *target);
        
        static std::unordered_map<target_t *, target_t *> guest_to_host;
        static std::unordered_map<target_t *, target_t *> host_to_guest;
        static std::shared_mutex mutex;

        static const offsets_t offsets;

        static R canonical(Args... args)  __attribute__((aligned(16))) {
            instance_info_t *instance_info;
            #if defined(_M_X86_64)
                asm("lea 1f(%%rip), %0" :"=r"(instance_info));
            #elif defined(_M_ARM_64)
                asm("adr %0, 1f" : "=r"(instance_info));
            #else
            #error unsupported arch
            #endif
            
            // force the label to exist
            asm goto (""::::data);

            return reinterpret_cast<marshaler_t *>(instance_info->marshaler)(args..., reinterpret_cast<target_t *>(instance_info->target));
            data:
                asm(".align 16\n 1: .quad 0xdeadbeefdeadbeef");            
                __builtin_unreachable();
        }
    };

    static int find_pattern(uint8_t *canonical_fn_ptr, uint64_t pattern, size_t size) {
        constexpr auto max_fn_size = 1024;
        
        for (int offset = 0; offset < max_fn_size; offset++ ) {
            if (memcmp(canonical_fn_ptr + offset, &pattern, size) == 0) {
                return offset;
            }
        }

        assert("Failed to find pattern in canonical function" && 0);
        return -1;
    }
    
    static offsets_t init_offsets(uint8_t *canonical_ptr) {
        auto end_offset = find_pattern(canonical_ptr, 0xDEADBEEFDEADBEEF, 8);

        return end_offset;
    }

    static void *make_instance(void *canonical, void *target, void *marshaler, offsets_t offsets) {
        auto canonical_fn_ptr = (uint8_t*)canonical;

        auto found_end_offset = offsets;

        assert("Failed to find end in canonical function" && found_end_offset != -1);

        [[maybe_unused]] auto align_minus_one = alignof(instance_info_t) - 1;

        assert("Canonical misalignment" && !(((uintptr_t)canonical) & align_minus_one));

        auto function_size = found_end_offset;

        assert("function_size misalignment" && !(function_size & align_minus_one));

        auto alloc_size = function_size + sizeof(instance_info_t);

        copyable_logf("function len: %d, alloc len: %d\n", (int)function_size, (int)alloc_size);

        static uint8_t *InstanceDataPtr;
        static size_t InstanceDataFree;
        static std::mutex InstanceDataMutex;

        uint8_t *instance;

        {
            std::lock_guard lk(InstanceDataMutex);
            if (InstanceDataFree < alloc_size) {
                InstanceDataFree = 16 * 1024;
                InstanceDataPtr = (uint8_t*)mmap(0, InstanceDataFree, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                assert(InstanceDataPtr != MAP_FAILED);
            }

            // alloc
            instance = InstanceDataPtr;
            InstanceDataFree -= alloc_size;
            InstanceDataPtr += alloc_size;
        }
        
        // copy
        memcpy(instance, canonical_fn_ptr, function_size);

        // bind
        auto code_offset = 0;
        auto info_offset = alloc_size - sizeof(instance_info_t);

        copyable_logf("code_offset offset: %d\n", (int)code_offset);
        copyable_logf("info_offset offset: %d\n", (int)info_offset);

        auto info = (instance_info_t*)(instance + info_offset);

        info->target = target;
        info->marshaler = marshaler;

        return (instance + code_offset);
    }

public:
    template<typename T>
    static T *make_instance(T *target, typename inner<T>::marshaler_t *marshaler);

    template<typename T>
    static T *get_target(T *bound_instance);
};


#define DECL_COPYABLE_TRAMPLOLINE(fn_type) \
template<> std::shared_mutex binder::inner<fn_type>::mutex{}; \
template<> std::unordered_map<fn_type *, fn_type *> binder::inner<fn_type>::guest_to_host{}; \
template<> std::unordered_map<fn_type *, fn_type *> binder::inner<fn_type>::host_to_guest{}; \
template<> const binder::offsets_t binder::inner<fn_type>::offsets = init_offsets(reinterpret_cast<uint8_t*>(&canonical)); \
template<> fn_type *binder::get_target<fn_type>(fn_type *bound_fn) { \
    std::shared_lock lk(binder::inner<fn_type>::mutex); \
    auto found = binder::inner<fn_type>::host_to_guest.find(bound_fn); \
    if (found != binder::inner<fn_type>::host_to_guest.end()) return found->second; \
    return nullptr; \
} \
template<> \
fn_type *binder::make_instance<fn_type>(fn_type *target, binder::inner<fn_type>::marshaler_t *marshaler) { \
    { \
        std::shared_lock lk(binder::inner<fn_type>::mutex); \
        auto found = binder::inner<fn_type>::guest_to_host.find(target); \
        if (found != binder::inner<fn_type>::guest_to_host.end()) return found->second;\
    } \
    auto rv = reinterpret_cast<fn_type *>(binder::make_instance(reinterpret_cast<void*>(&binder::inner<fn_type>::canonical), reinterpret_cast<void*>(target), reinterpret_cast<void*>(marshaler), binder::inner<fn_type>::offsets)); \
    { \
        std::lock_guard lk(binder::inner<fn_type>::mutex); \
        binder::inner<fn_type>::guest_to_host[target] = rv; \
        binder::inner<fn_type>::host_to_guest[rv] = target; \
    } \
    return rv; \
}
