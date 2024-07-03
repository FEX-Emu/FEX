set (MAN_DIR share/man CACHE PATH "MAN_DIR")

set (FEXCORE_BASE_SRCS
  Interface/Config/Config.cpp
  Utils/Allocator.cpp
  Utils/CPUInfo.cpp
  Utils/FileLoading.cpp
  Utils/ForcedAssert.cpp
  Utils/LogManager.cpp
  Utils/SpinWaitLock.cpp
  )

if (NOT MINGW_BUILD)
  list(APPEND FEXCORE_BASE_SRCS
    Utils/Allocator/64BitAllocator.cpp)
endif()

set (SRCS
  Common/JitSymbols.cpp
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
  Interface/Context/Context.cpp
  Interface/Core/LookupCache.cpp
  Interface/Core/BlockSamplingData.cpp
  Interface/Core/Core.cpp
  Interface/Core/CPUBackend.cpp
  Interface/Core/CPUID.cpp
  Interface/Core/Frontend.cpp
  Interface/Core/HostFeatures.cpp
  Interface/Core/ObjectCache/JobHandling.cpp
  Interface/Core/ObjectCache/NamedRegionObjectHandler.cpp
  Interface/Core/ObjectCache/ObjectCacheService.cpp
  Interface/Core/OpcodeDispatcher/AVX_128.cpp
  Interface/Core/OpcodeDispatcher/Crypto.cpp
  Interface/Core/OpcodeDispatcher/Flags.cpp
  Interface/Core/OpcodeDispatcher/Vector.cpp
  Interface/Core/OpcodeDispatcher/X87.cpp
  Interface/Core/OpcodeDispatcher/X87F64.cpp
  Interface/Core/OpcodeDispatcher.cpp
  Interface/Core/X86Tables.cpp
  Interface/Core/X86HelperGen.cpp
  Interface/Core/ArchHelpers/Arm64Emitter.cpp
  Interface/Core/Dispatcher/Dispatcher.cpp
  Interface/Core/Interpreter/Fallbacks/InterpreterFallbacks.cpp
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
  Interface/Core/JIT/Arm64/VectorOps.cpp
  Interface/Core/JIT/Arm64/Arm64Relocations.cpp
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
  Interface/HLE/Thunks/Thunks.cpp
  Interface/GDBJIT/GDBJIT.cpp
  Interface/IR/AOTIR.cpp
  Interface/IR/IRDumper.cpp
  Interface/IR/IREmitter.cpp
  Interface/IR/PassManager.cpp
  Interface/IR/Passes/ConstProp.cpp
  Interface/IR/Passes/DeadContextStoreElimination.cpp
  Interface/IR/Passes/IRDumperPass.cpp
  Interface/IR/Passes/IRValidation.cpp
  Interface/IR/Passes/RAValidation.cpp
  Interface/IR/Passes/RedundantFlagCalculationElimination.cpp
  Interface/IR/Passes/DeadStoreElimination.cpp
  Interface/IR/Passes/RegisterAllocationPass.cpp
  Utils/Telemetry.cpp
  Utils/Threads.cpp
  Utils/Profiler.cpp
  )

if (_M_ARM_64)
  list(APPEND SRCS Utils/ArchHelpers/Arm64.cpp)
else()
  list(APPEND SRCS Utils/ArchHelpers/Arm64_stubs.cpp)
endif()

if (ENABLE_GLIBC_ALLOCATOR_HOOK_FAULT)
  list(APPEND FEXCORE_BASE_SRCS
    Utils/AllocatorOverride.cpp)
endif()

set(DEFINES -DTHREAD_LOCAL=_Thread_local -DJIT_ARM64)

if (_M_X86_64)
  list(APPEND DEFINES -D_M_X86_64=1)
endif()

if (_M_ARM_64)
  list(APPEND DEFINES -D_M_ARM_64=1)
endif()

if (ENABLE_VIXL_SIMULATOR)
  # We can run the simulator on both x86-64 or AArch64 hosts
  list(APPEND DEFINES -DVIXL_SIMULATOR=1 -DVIXL_INCLUDE_SIMULATOR_AARCH64=1)
endif()

if (ENABLE_VIXL_DISASSEMBLER)
  list(APPEND DEFINES -DVIXL_DISASSEMBLER=1)
endif()

if (_M_ARM_64 AND HAS_CLANG_PRESERVE_ALL)
  list(APPEND DEFINES "-DFEXCORE_PRESERVE_ALL_ATTR=__attribute__((preserve_all));-DFEXCORE_HAS_PRESERVE_ALL_ATTR=1")
else()
  list(APPEND DEFINES "-DFEXCORE_PRESERVE_ALL_ATTR=;-DFEXCORE_HAS_PRESERVE_ALL_ATTR=0")
endif()

# Some defines for the softfloat library
list(APPEND DEFINES "-DSOFTFLOAT_BUILTIN_CLZ")

set (LIBS fmt::fmt vixl xxHash::xxhash FEXHeaderUtils CodeEmitter)

if (NOT MINGW_BUILD)
  list (APPEND LIBS dl)
else()
  list (APPEND LIBS synchronization)
  if (_M_ARM_64EC)
    list (APPEND LIBS kernelbase)
  endif()
endif()

if (ENABLE_JEMALLOC)
  list (APPEND LIBS FEX_jemalloc)
endif()

if (ENABLE_JEMALLOC_GLIBC_ALLOC)
  list (APPEND LIBS FEX_jemalloc_glibc)
endif()

# Generate config
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Interface/Config/Config.json.in
  ${CMAKE_BINARY_DIR}/generated/Config/Config.json)

# Generate IR include file
set(OUTPUT_IR_FOLDER "${CMAKE_BINARY_DIR}/include/FEXCore/IR")
set(OUTPUT_NAME "${OUTPUT_IR_FOLDER}/IRDefines.inc")
set(OUTPUT_DISPATCHER_NAME "${OUTPUT_IR_FOLDER}/IRDefines_Dispatch.inc")
set(INPUT_NAME "${CMAKE_CURRENT_SOURCE_DIR}/Interface/IR/IR.json")

file(MAKE_DIRECTORY "${OUTPUT_IR_FOLDER}")

add_custom_command(
  OUTPUT "${OUTPUT_NAME}" "${OUTPUT_DISPATCHER_NAME}"
  DEPENDS "${INPUT_NAME}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_generator.py"
  COMMAND "python3" "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_generator.py" "${INPUT_NAME}" "${OUTPUT_NAME}" "${OUTPUT_DISPATCHER_NAME}"
  )

set_source_files_properties(${OUTPUT_NAME} PROPERTIES
  GENERATED TRUE)

# Generate IR documentation
set(OUTPUT_IR_DOC "${CMAKE_BINARY_DIR}/IR.md")

add_custom_command(
  OUTPUT "${OUTPUT_IR_DOC}"
  DEPENDS "${INPUT_NAME}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_doc_generator.py"
  COMMAND "python3" "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_doc_generator.py" "${INPUT_NAME}" "${OUTPUT_IR_DOC}"
  )

set_source_files_properties(${OUTPUT_IR_NAME} PROPERTIES
  GENERATED TRUE)

# Create the target
add_custom_target(IR_INC
  DEPENDS "${OUTPUT_NAME}"
  DEPENDS "${OUTPUT_IR_DOC}")

# Generate the configuration include file
set(OUTPUT_CONFIG_FOLDER "${CMAKE_BINARY_DIR}/include/FEXCore/Config")
set(OUTPUT_CONFIG_NAME "${OUTPUT_CONFIG_FOLDER}/ConfigValues.inl")
set(OUTPUT_CONFIG_OPTION_NAME "${OUTPUT_CONFIG_FOLDER}/ConfigOptions.inl")
set(INPUT_CONFIG_NAME "${CMAKE_BINARY_DIR}/generated/Config/Config.json")
set(OUTPUT_MAN_NAME "${CMAKE_BINARY_DIR}/generated/FEX.1")
set(OUTPUT_MAN_NAME_COMPRESS "${CMAKE_BINARY_DIR}/generated/FEX.1.gz")

file(MAKE_DIRECTORY "${OUTPUT_CONFIG_FOLDER}")

add_custom_command(
  OUTPUT "${OUTPUT_CONFIG_NAME}"
  OUTPUT "${OUTPUT_CONFIG_OPTION_NAME}"
  OUTPUT "${OUTPUT_MAN_NAME}"
  DEPENDS "${INPUT_CONFIG_NAME}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/config_generator.py"
  COMMAND "python3" "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/config_generator.py" "${INPUT_CONFIG_NAME}" "${OUTPUT_CONFIG_NAME}" "${OUTPUT_MAN_NAME}"
  "${OUTPUT_CONFIG_OPTION_NAME}"
  )

add_custom_command(
  OUTPUT "${OUTPUT_MAN_NAME_COMPRESS}"
  DEPENDS "${OUTPUT_MAN_NAME}"
  COMMAND "gzip" "-kf9n" "${OUTPUT_MAN_NAME}"
  )

set_source_files_properties(${OUTPUT_CONFIG_NAME} PROPERTIES
  GENERATED TRUE)
set_source_files_properties(${OUTPUT_CONFIG_OPTION_NAME} PROPERTIES
  GENERATED TRUE)

set_source_files_properties(${OUTPUT_MAN_NAME} PROPERTIES
  GENERATED TRUE)
set_source_files_properties(${OUTPUT_MAN_NAME_COMPRESS} PROPERTIES
  GENERATED TRUE)

# Create the target
add_custom_target(CONFIG_INC
  DEPENDS "${OUTPUT_CONFIG_NAME}"
  DEPENDS "${OUTPUT_CONFIG_OPTION_NAME}"
  DEPENDS "${OUTPUT_MAN_NAME}"
  DEPENDS "${OUTPUT_MAN_NAME_COMPRESS}")

# Install the compressed man page
install(FILES ${OUTPUT_MAN_NAME_COMPRESS} DESTINATION ${MAN_DIR}/man1)

# Add in diagnostic colours if the option is available.
# Ninja code generator will kill colours if this isn't here
check_cxx_compiler_flag(-fdiagnostics-color=always GCC_COLOR)
check_cxx_compiler_flag(-fcolor-diagnostics CLANG_COLOR)

function(AddDefaultOptionsToTarget Name)
  set_target_properties(${Name} PROPERTIES C_VISIBILITY_PRESET hidden)
  set_target_properties(${Name} PROPERTIES CXX_VISIBILITY_PRESET hidden)
  set_target_properties(${Name} PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)
  target_include_directories(${Name} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

  target_include_directories(${Name} PRIVATE IncludePrivate/)
  target_include_directories(${Name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)

  target_include_directories(${Name} PUBLIC "${PROJECT_SOURCE_DIR}/include/")
  target_include_directories(${Name} PUBLIC "${CMAKE_BINARY_DIR}/include/")

  target_compile_definitions(${Name} PRIVATE ${DEFINES})
  add_dependencies(${Name} CONFIG_INC IR_INC)

  target_compile_options(${Name}
    PRIVATE
    -Wall
    -Werror=cast-qual
    -Werror=ignored-qualifiers
    -Werror=implicit-fallthrough

    -Wno-trigraphs
    -ffunction-sections
    -fwrapv
  )

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

  if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
    target_link_options(${Name}
      PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed"
    )
  endif()
endfunction()

# Build FEXCore_Config static library
add_library(FEXCore_Base STATIC ${FEXCORE_BASE_SRCS})
target_link_libraries(FEXCore_Base ${LIBS})
AddDefaultOptionsToTarget(FEXCore_Base)

function(AddObject Name Type)
  add_library(${Name} ${Type} ${SRCS})

  target_link_libraries(${Name} FEXCore_Base)
  target_compile_options(${Name} PRIVATE ${FEX_TUNE_COMPILE_FLAGS})
  AddDefaultOptionsToTarget(${Name})

  set_target_properties(${Name} PROPERTIES OUTPUT_NAME FEXCore)
endfunction()

function(AddLibrary Name Type)
  add_library(${Name} ${Type} $<TARGET_OBJECTS:${PROJECT_NAME}_object>)
  target_link_libraries(${Name} FEXCore_Base)
  target_compile_options(${Name} PRIVATE ${FEX_TUNE_COMPILE_FLAGS})
  set_target_properties(${Name} PROPERTIES OUTPUT_NAME FEXCore)

  AddDefaultOptionsToTarget(${Name})
endfunction()

AddObject(${PROJECT_NAME}_object OBJECT)
AddLibrary(${PROJECT_NAME} STATIC)
AddLibrary(${PROJECT_NAME}_shared SHARED)

if (NOT MINGW_BUILD)
  install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_shared
    LIBRARY
      DESTINATION lib
      COMPONENT Libraries
    ARCHIVE
      DESTINATION lib
      COMPONENT Libraries)
endif()
