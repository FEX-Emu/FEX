// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace FEXCore::IR {
enum IROps : uint16_t;
}

namespace FEXCore::CPU {

// Base template for fallback handling.
//
// Registering and hooking up fallback is currently like so:
//
// 1. Go to InterpreterFallbacks.cpp and create a template specialization of
//    the GetFallbackInfo member function.
//
//    This member function should reasonably define what the fallback you're
//    going to create will take as parameters and return as a result. For example:
//
//    template<>
//    FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(double), Core::FallbackHandlerIndex Index) {
//      return {FABI_F80_F64, (void*)fn, Index};
//    }
//
//    Defines info about a fallback that takes a double as an argument and
//    returns a X80SoftFloat instance.
//
//    You will also want to define a new FallbackHandlerIndex enum member and use it
//    to set up the new info handler into the Info array in FillFallbackIndexPointers.
//
// 1.1. (potentially optional). Define a new ABI element in the FallbackAPI enum.
//      This ABI enum value will be used to tell the JITs how to handle the fallback
//      properly. These enum values specify the return type followed by its argument types.
//
//      So, FABI_I64_F80_F80, for example indicates that the function will behave like a
//      function as if were defined as:
//
//      uint64_t fn(X80SoftFloat, X80SoftFloat)
//
// 1.2. (potentially optional). If you needed to define a new enum ABI type like in 1.1, then
//      you need to add the handling for it in the JITs, which can be found in the respective
//      JIT's JIT.cpp file in a function called Op_Unhandled
//
//      You need to add a new case to the ABI switch statement using the new ABI type
//      and do the necessary moving of data from register-allocated JIT parameters
//      into that platform's registers that respects the calling convention. After this is
//      done, most of the necessary background boilerplate is finished.
//
// 2. Now, make a specialization of this class with a member function named 'handle()'
//    that takes the same parameters as the ones described in the fallback info function
//    specialization.
//
//    For example, if you have the fallback info from the example in step 1, it would be:
//
//    template <>
//    struct OpHandlers<IR::CoolNewIROpcode> {
//      static X80SoftFloat handle(double src) {
//        return ...;
//      }
//    };
//
// 3. Fill out the behavior of the OpHandler specialization to perform what you would like
//    the fallback to do.
//
// 4. Add an implementation of the IR op to the Interpreter that passes through to the
//    OpHandler implementation.
//
// 5. Done.
//
template<IR::IROps Op>
struct OpHandlers {};

} // namespace FEXCore::CPU
