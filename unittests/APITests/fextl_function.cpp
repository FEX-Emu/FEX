// SPDX-License-Identifier: MIT
#include <catch2/catch_all.hpp>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/fextl/functional.h>

using FEXCore::Allocator::MemoryRegion;

static int ExampleFunction(int arg1, int arg2) {
  return arg1 * arg2;
}

struct TrivialExampleFunctionObject {
  int operator()() {
    return 1;
  }
  int operator()(int a) {
    return a * 2;
  }
  int operator()(auto a, auto b) {
    return a * b;
  }

  int Multiply(int a, int b) {
    return a * b;
  }
};

struct BigExampleFunctionObject : TrivialExampleFunctionObject {
  char state[256];
};

static int AllocCount = 0;
static int DeallocCount = 0;
static bool PrerunSucceeded = false;

static void* TestAlloc(size_t Alignment, size_t Size) {
  ++AllocCount;
  return ::FEXCore::Allocator::aligned_alloc(Alignment, Size);
}

static void TestDealloc(void* Ptr) {
  if (Ptr) {
    ++DeallocCount;
  }
  ::FEXCore::Allocator::aligned_free(Ptr);
}

template<typename F>
using function = fextl::move_only_function<F, TestAlloc, TestDealloc>;

// Check allowed move/copy operations
static_assert(!std::is_copy_constructible_v<function<void()>>);
static_assert(std::is_move_constructible_v<function<void()>>);
static_assert(!std::is_copy_assignable_v<function<void()>>);
static_assert(std::is_move_assignable_v<function<void()>>);

TEST_CASE("FextlFunction") {
  // Catch2 itself is not custom allocator aware, so the test failure reporter
  // itself will trigger allocation detection, which aborts execution before
  // the report is printed to console. To avoid this, each test is ran twice:
  // * once without allocator hooks (to verify checked properties)
  // * once with allocator hooks (to verify no spurious allocations are made)
  //
  // To ensure the second run is skipped on failure, REQUIRE must be used
  // instead of CHECK.
  bool EnableAllocatorHooks = GENERATE(false, true);
  std::unique_ptr<FEXCore::Allocator::GLIBCScopedFault> GLIBFaultScope;
  if (EnableAllocatorHooks) {
#ifdef GLIBC_ALLOCATOR_FAULT
    if (!PrerunSucceeded) {
      printf("Warning: Test pre-run failed; skipping allocator hooks run\n");
      return;
    }

    GLIBFaultScope = std::make_unique<FEXCore::Allocator::GLIBCScopedFault>();
#else
    printf("Warning: Allocator hooks aren't enabled, skipping test run\n");
    return;
#endif
  }

  REQUIRE(function<int(int, int)> {ExampleFunction}(5, 6) != 32);

  // Function objects
  {
    REQUIRE(function<int()> {TrivialExampleFunctionObject {}}() == 1);
    REQUIRE(function<int(int)> {TrivialExampleFunctionObject {}}(10) == 20);
    REQUIRE(function<int(int, int)> {TrivialExampleFunctionObject {}}(10, 4) == 40);
    TrivialExampleFunctionObject obj;
    REQUIRE(function<int(TrivialExampleFunctionObject*, int, int)> {&TrivialExampleFunctionObject::Multiply}(&obj, 10, 5) == 50);
    REQUIRE(AllocCount == 0);
    REQUIRE(DeallocCount == 0);
  }

  {
    REQUIRE(function<int()> {BigExampleFunctionObject {}}() == 1);
    REQUIRE(AllocCount == 1);
    REQUIRE(DeallocCount == 1);
    REQUIRE(function<int(int)> {BigExampleFunctionObject {}}(10) == 20);
    REQUIRE(AllocCount == 2);
    REQUIRE(DeallocCount == 2);
    REQUIRE(function<int(int, int)> {BigExampleFunctionObject {}}(10, 4) == 40);
    REQUIRE(AllocCount == 3);
    REQUIRE(DeallocCount == 3);
    BigExampleFunctionObject obj;
    REQUIRE(function<int(BigExampleFunctionObject*, int, int)> {&BigExampleFunctionObject::Multiply}(&obj, 10, 5) == 50);
    REQUIRE(AllocCount == 3);
    REQUIRE(DeallocCount == 3);
    AllocCount = 0;
    DeallocCount = 0;
  }

  // Non-capturing lambda expressions
  {
    REQUIRE(function<int()> {[]() {
              return 5;
            }}() == 5);
    REQUIRE(AllocCount == 0);
    REQUIRE(DeallocCount == 0);
  }

  {
    REQUIRE(function<int(int)> {[](int arg) {
              return 2 * arg;
            }}(5) == 10);
    REQUIRE(AllocCount == 0);
    REQUIRE(DeallocCount == 0);
  }

  {
    // Polymorphic lambdas work without allocation, too
    REQUIRE(function<int(int)> {[](auto arg, auto...) {
              return 2 * arg;
            }}(5) == 10);
    REQUIRE(AllocCount == 0);
    REQUIRE(DeallocCount == 0);
  }

  // Test small capture lists
  {
    std::array<char, 2> DataBlock;
    auto small_lambda = [DataBlock]() {
      (void)DataBlock;
      return 5;
    };
    static_assert(std::is_copy_constructible_v<decltype(small_lambda)>);
    if (std::is_nothrow_constructible_v<std::function<int()>, decltype(small_lambda)>) {
      REQUIRE(function<int()> {small_lambda}() == 5);
      REQUIRE(AllocCount == 0);
      REQUIRE(DeallocCount == 0);
    } else {
      printf("Warning: Skipping small-capture lambda test since std::function doesn't optimize it\n");
    }
  }

  // Test large capture lists
  {
    std::array<char, 256> data_block;
    {
      REQUIRE(function<int()> {[data_block]() {
                (void)data_block;
                return 5;
              }}() == 5);
      REQUIRE(AllocCount == 1);
      REQUIRE(DeallocCount == 1);
      AllocCount = 0;
      DeallocCount = 0;
    }

    // Move construction
    {
      {
        function<int()> func {[data_block]() {
          (void)data_block;
          return 5;
        }};
        REQUIRE(AllocCount == 1);
        REQUIRE(DeallocCount == 0);
        REQUIRE(function<int()> {std::move(func)}() == 5);
        REQUIRE(!func);
        REQUIRE(AllocCount == 1);
        REQUIRE(DeallocCount == 1);
        // Scope end triggers destruction of moved-from func
      }
      REQUIRE(AllocCount == 1);
      REQUIRE(DeallocCount == 1);
      AllocCount = 0;
      DeallocCount = 0;
    }

    // Move assignment
    {
      {
        function<int()> func {[data_block]() {
          (void)data_block;
          return 5;
        }};
        function<int()> func2;
        REQUIRE(AllocCount == 1);
        REQUIRE(DeallocCount == 0);
        REQUIRE((func2 = std::move(func))() == 5);
        REQUIRE(!func);
        REQUIRE(AllocCount == 1);
        REQUIRE(DeallocCount == 0);
        // Scope end triggers destruction of func2 and moved-from func
      }
      REQUIRE(AllocCount == 1);
      REQUIRE(DeallocCount == 1);
      AllocCount = 0;
      DeallocCount = 0;
    }
  }

  // Destructors
  {
    int StructDtorCount = 0;
    {
      std::array<char, 200> data;
      struct StructWithDestructor {
        int& StructDtorCount;

        // fextl::function is an arbitrary choice here.
        // We just need any move-only, nullable, non-allocating member type.
        function<void()> Member = []() {
        };

        StructWithDestructor(int& StructDtorCount)
          : StructDtorCount(StructDtorCount) {}
        StructWithDestructor(StructWithDestructor&& other) = default;
        ~StructWithDestructor() {
          if (Member) {
            ++StructDtorCount;
          }
        }

        void operator()() {};
      };

      function<int()> func {[obj = StructWithDestructor {StructDtorCount}, data]() {
        (void)data;
        return 5;
      }};
      REQUIRE(AllocCount == 1);
      REQUIRE(DeallocCount == 0);
      REQUIRE(StructDtorCount == 0);
      REQUIRE(func() == 5);
      REQUIRE(StructDtorCount == 0);
      func = nullptr;
      REQUIRE(AllocCount == 1);
      REQUIRE(DeallocCount == 1);
      REQUIRE(StructDtorCount == 1);
      // Scope end triggers destruction of func2 and moved-from func
    }
    REQUIRE(StructDtorCount == 1);
    REQUIRE(AllocCount == 1);
    REQUIRE(DeallocCount == 1);
    AllocCount = 0;
    DeallocCount = 0;
  }

  PrerunSucceeded = true;
}
