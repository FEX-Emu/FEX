if (ENABLE_CLANG_FORMAT)
  find_program(CLANG_TIDY_EXE "clang-tidy")
  set(CLANG_TIDY_FLAGS
    "-checks=*"
    "-fuchsia*"
    "-bugprone-macro-parentheses"
    "-clang-analyzer-core.*"
    "-cppcoreguidelines-pro-type-*"
    "-cppcoreguidelines-pro-bounds-array-to-pointer-decay"
    "-cppcoreguidelines-pro-bounds-pointer-arithmetic"
    "-cppcoreguidelines-avoid-c-arrays"
    "-cppcoreguidelines-avoid-magic-numbers"
    "-cppcoreguidelines-pro-bounds-constant-array-index"
    "-cppcoreguidelines-no-malloc"
    "-cppcoreguidelines-special-member-functions"
    "-cppcoreguidelines-owning-memory"
    "-cppcoreguidelines-macro-usage"
    "-cppcoreguidelines-avoid-goto"
    "-google-readability-function-size"
    "-google-readability-namespace-comments"
    "-google-readability-braces-around-statements"
    "-google-build-using-namespace"
    "-hicpp-*"
    "-llvm-namespace-comment"
    "-llvm-include-order" # Messes up with case sensitivity
    "-misc-unused-parameters"
    "-modernize-loop-convert"
    "-modernize-use-auto"
    "-modernize-avoid-c-arrays"
    "-modernize-use-nodiscard"
    "readability-*"
    "-readability-function-size"
    "-readability-implicit-bool-conversion"
    "-readability-braces-around-statements"
    "-readability-else-after-return"
    "-readability-magic-numbers"
    "-readability-named-parameter"
    "-readability-uppercase-literal-suffix"
    "-cert-err34-c"
    "-cert-err58-cpp"
    "-bugprone-exception-escape"
    )
  string(REPLACE ";" "," CLANG_TIDY_FLAGS "${CLANG_TIDY_FLAGS}")

  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE} "${CLANG_TIDY_FLAGS}")
endif()

set (SRCS
  Common/Paths.cpp
  Common/JitSymbols.cpp
  Common/NetStream.cpp
  Common/SoftFloat-3e/extF80_add.c
  Common/SoftFloat-3e/extF80_div.c
  Common/SoftFloat-3e/extF80_sub.c
  Common/SoftFloat-3e/extF80_mul.c
  Common/SoftFloat-3e/extF80_rem.c
  Common/SoftFloat-3e/extF80_sqrt.c
  Common/SoftFloat-3e/s_add128.c
  Common/SoftFloat-3e/s_sub128.c
  Common/SoftFloat-3e/s_le128.c
  Common/SoftFloat-3e/extF80_to_i32.c
  Common/SoftFloat-3e/extF80_to_i64.c
  Common/SoftFloat-3e/extF80_to_ui64.c
  Common/SoftFloat-3e/extF80_to_f32.c
  Common/SoftFloat-3e/extF80_to_f64.c
  Common/SoftFloat-3e/i32_to_extF80.c
  Common/SoftFloat-3e/ui64_to_extF80.c
  Common/SoftFloat-3e/extF80_to_f128.c
  Common/SoftFloat-3e/f128_to_extF80.c
  Common/SoftFloat-3e/s_roundToUI64.c
  Common/SoftFloat-3e/s_f128UIToCommonNaN.c
  Common/SoftFloat-3e/s_commonNaNToF128UI.c
  Common/SoftFloat-3e/s_shortShiftRight128.c
  Common/SoftFloat-3e/s_normSubnormalF128Sig.c
  Common/SoftFloat-3e/s_roundToI32.c
  Common/SoftFloat-3e/s_roundToI64.c
  Common/SoftFloat-3e/s_roundPackToF32.c
  Common/SoftFloat-3e/s_addMagsExtF80.c
  Common/SoftFloat-3e/s_extF80UIToCommonNaN.c
  Common/SoftFloat-3e/s_commonNaNToF32UI.c
  Common/SoftFloat-3e/s_commonNaNToF64UI.c
  Common/SoftFloat-3e/s_shortShiftRightJam64.c
  Common/SoftFloat-3e/s_shortShiftRightJam64Extra.c
  Common/SoftFloat-3e/s_shiftRightJam64Extra.c
  Common/SoftFloat-3e/s_shortShiftRightJam64Extra.c
  Common/SoftFloat-3e/s_roundPackToF64.c
  Common/SoftFloat-3e/s_propagateNaNExtF80UI.c
  Common/SoftFloat-3e/s_roundPackToExtF80.c
  Common/SoftFloat-3e/s_normSubnormalExtF80Sig.c
  Common/SoftFloat-3e/s_shiftRightJam64.c
  Common/SoftFloat-3e/s_subMagsExtF80.c
  Common/SoftFloat-3e/s_shiftRightJam32.c
  Common/SoftFloat-3e/s_shiftRightJam128.c
  Common/SoftFloat-3e/s_shiftRightJam128Extra.c
  Common/SoftFloat-3e/s_normRoundPackToExtF80.c
  Common/SoftFloat-3e/s_shortShiftLeft128.c
  Common/SoftFloat-3e/s_approxRecip32_1.c
  Common/SoftFloat-3e/s_approxRecip_1Ks.c
  Common/SoftFloat-3e/s_approxRecipSqrt32_1.c
  Common/SoftFloat-3e/s_approxRecipSqrt_1Ks.c
  Common/SoftFloat-3e/softfloat_raiseFlags.c
  Common/SoftFloat-3e/softfloat_state.c
  Common/SoftFloat-3e/f64_to_extF80.c
  Common/SoftFloat-3e/s_commonNaNToExtF80UI.c
  Common/SoftFloat-3e/s_normSubnormalF64Sig.c
  Common/SoftFloat-3e/s_f64UIToCommonNaN.c
  Common/SoftFloat-3e/extF80_roundToInt.c
  Common/SoftFloat-3e/extF80_eq.c
  Common/SoftFloat-3e/extF80_lt.c
  Common/SoftFloat-3e/s_lt128.c
  Common/SoftFloat-3e/s_mul64ByShifted32To128.c
  Common/SoftFloat-3e/s_mul64To128.c
  Common/SoftFloat-3e/s_countLeadingZeros8.c
  Common/SoftFloat-3e/s_countLeadingZeros32.c
  Common/SoftFloat-3e/s_countLeadingZeros64.c
  Common/SoftFloat-3e/f32_to_extF80.c
  Common/SoftFloat-3e/s_normSubnormalF32Sig.c
  Common/SoftFloat-3e/s_f32UIToCommonNaN.c
  Interface/Config/Config.cpp
  Interface/Context/Context.cpp
  Interface/Core/BlockCache.cpp
  Interface/Core/BlockSamplingData.cpp
  Interface/Core/Core.cpp
  Interface/Core/CPUID.cpp
  Interface/Core/Frontend.cpp
  Interface/Core/GdbServer.cpp
  Interface/Core/HostFeatures.cpp
  Interface/Core/OpcodeDispatcher.cpp
  Interface/Core/SignalDelegator.cpp
  Interface/Core/X86Tables.cpp
  Interface/Core/X86DebugInfo.cpp
  Interface/Core/X86HelperGen.cpp
  Interface/Core/Interpreter/InterpreterCore.cpp
  Interface/Core/X86Tables/BaseTables.cpp
  Interface/Core/X86Tables/DDDTables.cpp
  Interface/Core/X86Tables/EVEXTables.cpp
  Interface/Core/X86Tables/H0F38Tables.cpp
  Interface/Core/X86Tables/H0F3ATables.cpp
  Interface/Core/X86Tables/PrimaryGroupTables.cpp
  Interface/Core/X86Tables/SecondaryGroupTables.cpp
  Interface/Core/X86Tables/SecondaryModRMTables.cpp
  Interface/Core/X86Tables/SecondaryTables.cpp
  Interface/Core/X86Tables/VEXTables.cpp
  Interface/Core/X86Tables/X87Tables.cpp
  Interface/Core/X86Tables/XOPTables.cpp
  Interface/HLE/FileManagement.cpp
  Interface/HLE/EmulatedFiles/EmulatedFiles.cpp
  Interface/HLE/Syscalls.cpp
  Interface/HLE/x32/Syscalls.cpp
  Interface/HLE/x32/FD.cpp
  Interface/HLE/x32/FS.cpp
  Interface/HLE/x32/Info.cpp
  Interface/HLE/x32/Memory.cpp
  Interface/HLE/x32/NotImplemented.cpp
  Interface/HLE/x32/Semaphore.cpp
  Interface/HLE/x32/Thread.cpp
  Interface/HLE/x64/FD.cpp
  Interface/HLE/x64/IO.cpp
  Interface/HLE/x64/Ioctl.cpp
  Interface/HLE/x64/Info.cpp
  Interface/HLE/x64/Memory.cpp
  Interface/HLE/x64/Msg.cpp
  Interface/HLE/x64/NotImplemented.cpp
  Interface/HLE/x64/Semaphore.cpp
  Interface/HLE/x64/Sched.cpp
  Interface/HLE/x64/Socket.cpp
  Interface/HLE/x64/Thread.cpp
  Interface/HLE/x64/Syscalls.cpp
  Interface/HLE/x64/Time.cpp
  Interface/HLE/Syscalls/EPoll.cpp
  Interface/HLE/Syscalls/FD.cpp
  Interface/HLE/Syscalls/FS.cpp
  Interface/HLE/Syscalls/Info.cpp
  Interface/HLE/Syscalls/IO.cpp
  Interface/HLE/Syscalls/Key.cpp
  Interface/HLE/Syscalls/Memory.cpp
  Interface/HLE/Syscalls/Msg.cpp
  Interface/HLE/Syscalls/Sched.cpp
  Interface/HLE/Syscalls/Semaphore.cpp
  Interface/HLE/Syscalls/SHM.cpp
  Interface/HLE/Syscalls/Signals.cpp
  Interface/HLE/Syscalls/Socket.cpp
  Interface/HLE/Syscalls/Thread.cpp
  Interface/HLE/Syscalls/Time.cpp
  Interface/HLE/Syscalls/Timer.cpp
  Interface/HLE/Syscalls/NotImplemented.cpp
  Interface/HLE/Syscalls/Stubs.cpp
  Interface/HLE/Thunks/Thunks.cpp
  Interface/Memory/MemMapper.cpp
  Interface/Memory/SharedMem.cpp
  Interface/IR/IR.cpp
  Interface/IR/IREmitter.cpp
  Interface/IR/PassManager.cpp
  Interface/IR/Passes/ConstProp.cpp
  Interface/IR/Passes/DeadCodeElimination.cpp
  Interface/IR/Passes/DeadContextStoreElimination.cpp
  Interface/IR/Passes/IRCompaction.cpp
  Interface/IR/Passes/IRValidation.cpp
  Interface/IR/Passes/ValueDominanceValidation.cpp
  Interface/IR/Passes/PhiValidation.cpp
  Interface/IR/Passes/RedundantFlagCalculationElimination.cpp
  Interface/IR/Passes/DeadFlagStoreElimination.cpp
  Interface/IR/Passes/DeadGPRStoreElimination.cpp
  Interface/IR/Passes/RegisterAllocationPass.cpp
  Interface/IR/Passes/SyscallOptimization.cpp
  )

if (_M_X86_64)
  list(APPEND SRCS Interface/Core/Interpreter/x86_64Dispatcher.cpp)
endif()
if(_M_ARM_64)
  list(APPEND SRCS Interface/Core/Interpreter/Arm64Dispatcher.cpp)
endif()

set (JIT_LIBS )
if (ENABLE_JIT)
  if (_M_X86_64)
    add_definitions(-D_M_X86_64=1)
    if (NOT FORCE_AARCH64)
      list(APPEND SRCS
        Interface/Core/JIT/x86_64/JIT.cpp
        Interface/Core/JIT/x86_64/ALUOps.cpp
        Interface/Core/JIT/x86_64/AtomicOps.cpp
        Interface/Core/JIT/x86_64/BranchOps.cpp
        Interface/Core/JIT/x86_64/ConversionOps.cpp
        Interface/Core/JIT/x86_64/EncryptionOps.cpp
        Interface/Core/JIT/x86_64/FlagOps.cpp
        Interface/Core/JIT/x86_64/MemoryOps.cpp
        Interface/Core/JIT/x86_64/MiscOps.cpp
        Interface/Core/JIT/x86_64/MoveOps.cpp
        Interface/Core/JIT/x86_64/VectorOps.cpp)
    endif()
  endif()
  if(_M_ARM_64)
    add_definitions(-D_M_ARM_64=1)
    add_definitions(-DVIXL_INCLUDE_TARGET_AARCH64=1)
    add_definitions(-DVIXL_CODE_BUFFER_MMAP=1)
    list(APPEND SRCS
      Interface/Core/JIT/Arm64/JIT.cpp
      Interface/Core/JIT/Arm64/ALUOps.cpp
      Interface/Core/JIT/Arm64/AtomicOps.cpp
      Interface/Core/JIT/Arm64/BranchOps.cpp
      Interface/Core/JIT/Arm64/ConversionOps.cpp
      Interface/Core/JIT/Arm64/EncryptionOps.cpp
      Interface/Core/JIT/Arm64/FlagOps.cpp
      Interface/Core/JIT/Arm64/MemoryOps.cpp
      Interface/Core/JIT/Arm64/MiscOps.cpp
      Interface/Core/JIT/Arm64/MoveOps.cpp
      Interface/Core/JIT/Arm64/VectorOps.cpp)
    list(APPEND JIT_LIBS vixl)
  endif()
endif()

if (ENABLE_JITSYMBOLS)
  add_definitions(-DENABLE_JITSYMBOLS=1)
endif()

# Generate IR include file
set(OUTPUT_IR_FOLDER "${CMAKE_BINARY_DIR}/include/FEXCore/IR")
set(OUTPUT_NAME "${OUTPUT_IR_FOLDER}/IRDefines.inc")
set(INPUT_NAME "${CMAKE_CURRENT_SOURCE_DIR}/Interface/IR/IR.json")

add_custom_target(CREATE_IR_FOLDER ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_IR_FOLDER}")

add_custom_command(
  OUTPUT "${OUTPUT_NAME}"
  DEPENDS "${INPUT_NAME}"
  DEPENDS CREATE_IR_FOLDER
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_generator.py"
  COMMAND "python3" "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_generator.py" "${INPUT_NAME}" "${OUTPUT_NAME}"
  )

set_source_files_properties(${OUTPUT_NAME} PROPERTIES
  GENERATED TRUE)

# Generate IR documentation
set(OUTPUT_IR_DOC "${CMAKE_BINARY_DIR}/IR.md")

add_custom_command(
  OUTPUT "${OUTPUT_IR_DOC}"
  DEPENDS "${INPUT_NAME}"
  DEPENDS CREATE_IR_FOLDER
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_doc_generator.py"
  COMMAND "python3" "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_doc_generator.py" "${INPUT_NAME}" "${OUTPUT_IR_DOC}"
  )

set_source_files_properties(${OUTPUT_IR_NAME} PROPERTIES
  GENERATED TRUE)

# Create teh target
add_custom_target(IR_INC
  DEPENDS "${OUTPUT_NAME}"
  DEPENDS "${OUTPUT_IR_DOC}")

# Add in diagnostic colours if the option is available.
# Ninja code generator will kill colours if this isn't here
check_cxx_compiler_flag(-fdiagnostics-color=always GCC_COLOR)
check_cxx_compiler_flag(-fcolor-diagnostics CLANG_COLOR)

set(LINUX_LIBS
  numa)

function(AddLibrary Name Type)
  add_library(${Name} ${Type} ${SRCS})
  add_dependencies(${Name} IR_INC)
  target_link_libraries(${Name} pthread rt ${JIT_LIBS} ${LINUX_LIBS} dl)
  set_target_properties(${Name} PROPERTIES OUTPUT_NAME FEXCore)

  target_include_directories(${Name} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

  target_include_directories(${Name} PRIVATE IncludePrivate/)
  target_include_directories(${Name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)

  target_include_directories(${Name} PUBLIC "${PROJECT_SOURCE_DIR}/include/")
  target_include_directories(${Name} PUBLIC "${CMAKE_BINARY_DIR}/include/")

  target_compile_options(${Name}
    PRIVATE
    -Wno-trigraphs -Wall)

  if (GCC_COLOR)
    target_compile_options(${Name}
      PRIVATE
      "-fdiagnostics-color=always")
  endif()
  if (CLANG_COLOR)
    target_compile_options(${Name}
      PRIVATE
      "-fcolor-diagnostics")
  endif()
endfunction()

AddLibrary(${PROJECT_NAME} STATIC)
AddLibrary(${PROJECT_NAME}_shared SHARED)

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_shared
  LIBRARY
    DESTINATION lib
    COMPONENT Libraries
  ARCHIVE
    DESTINATION lib
    COMPONENT Libraries)
