set (MAN_DIR ${CMAKE_INSTALL_PREFIX}/share/man CACHE PATH "MAN_DIR")

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
  Interface/Core/LookupCache.cpp
  Interface/Core/BlockSamplingData.cpp
  Interface/Core/CompileService.cpp
  Interface/Core/Core.cpp
  Interface/Core/CPUID.cpp
  Interface/Core/Frontend.cpp
  Interface/Core/GdbServer.cpp
  Interface/Core/HostFeatures.cpp
  Interface/Core/OpcodeDispatcher.cpp
  Interface/Core/X86Tables.cpp
  Interface/Core/X86DebugInfo.cpp
  Interface/Core/X86HelperGen.cpp
  Interface/Core/ArchHelpers/Arm64_stubs.cpp
  Interface/Core/ArchHelpers/Arm64Emitter.cpp
  Interface/Core/Dispatcher/Dispatcher.cpp
  Interface/Core/Dispatcher/X86Dispatcher.cpp
  Interface/Core/Dispatcher/Arm64Dispatcher.cpp
  Interface/Core/Interpreter/InterpreterCore.cpp
  Interface/Core/Interpreter/InterpreterOps.cpp
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
  Interface/IR/IRDumper.cpp
  Interface/IR/IRParser.cpp
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
  Interface/IR/Passes/DeadStoreElimination.cpp
  Interface/IR/Passes/StaticRegisterAllocationPass.cpp
  Interface/IR/Passes/RegisterAllocationPass.cpp
  Interface/IR/Passes/SyscallOptimization.cpp
  Utils/ELFContainer.cpp
  Utils/ELFSymbolDatabase.cpp
  Utils/LogManager.cpp
  Utils/Threads.cpp
  )

if(_M_ARM_64)
  list(APPEND SRCS
    Interface/Core/ArchHelpers/Arm64.cpp)
endif()

set(DEFINES )

if (_M_X86_64)
  list(APPEND DEFINES -D_M_X86_64=1)
endif()

if (_M_ARM_64)
  list(APPEND DEFINES -D_M_ARM_64=1)
endif()

if (ENABLE_JIT_X86_64)
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
  list(APPEND DEFINES -DJIT_X86_64)
endif()

if (ENABLE_JIT_ARM64)
  list(APPEND DEFINES -DJIT_ARM64)
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
endif()

if (ENABLE_JITSYMBOLS)
  list(APPEND DEFINES -DENABLE_JITSYMBOLS=1)
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

# Create the target
add_custom_target(IR_INC
  DEPENDS "${OUTPUT_NAME}"
  DEPENDS "${OUTPUT_IR_DOC}")

# Generate the configuration include file
set(OUTPUT_CONFIG_FOLDER "${CMAKE_BINARY_DIR}/include/FEXCore/Config")
set(OUTPUT_CONFIG_NAME "${OUTPUT_CONFIG_FOLDER}/ConfigValues.inl")
set(OUTPUT_CONFIG_OPTION_NAME "${OUTPUT_CONFIG_FOLDER}/ConfigOptions.inl")
set(INPUT_CONFIG_NAME "${CMAKE_CURRENT_SOURCE_DIR}/Interface/Config/Config.json")
set(OUTPUT_MAN_NAME "${CMAKE_BINARY_DIR}/generated/FEX.1")

add_custom_target(CREATE_CONFIG_FOLDER ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_CONFIG_FOLDER}")

add_custom_command(
  OUTPUT "${OUTPUT_CONFIG_NAME}"
  OUTPUT "${OUTPUT_CONFIG_OPTION_NAME}"
  OUTPUT "${OUTPUT_MAN_NAME}"
  DEPENDS "${INPUT_CONFIG_NAME}"
  DEPENDS CREATE_CONFIG_FOLDER
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/config_generator.py"
  COMMAND "python3" "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/config_generator.py" "${INPUT_CONFIG_NAME}" "${OUTPUT_CONFIG_NAME}" "${OUTPUT_MAN_NAME}"
  "${OUTPUT_CONFIG_OPTION_NAME}"
  )

set_source_files_properties(${OUTPUT_CONFIG_NAME} PROPERTIES
  GENERATED TRUE)
set_source_files_properties(${OUTPUT_CONFIG_OPTION_NAME} PROPERTIES
  GENERATED TRUE)

set_source_files_properties(${OUTPUT_MAN_NAME} PROPERTIES
  GENERATED TRUE)

# Create the target
add_custom_target(CONFIG_INC
  DEPENDS "${OUTPUT_CONFIG_NAME}"
  DEPENDS "${OUTPUT_CONFIG_OPTION_NAME}"
  DEPENDS "${OUTPUT_MAN_NAME}")

# Install the man page
install(FILES ${OUTPUT_MAN_NAME} DESTINATION ${MAN_DIR}/man1)

# Add in diagnostic colours if the option is available.
# Ninja code generator will kill colours if this isn't here
check_cxx_compiler_flag(-fdiagnostics-color=always GCC_COLOR)
check_cxx_compiler_flag(-fcolor-diagnostics CLANG_COLOR)

function(AddObject Name Type)
  add_library(${Name} ${Type} ${SRCS})
  add_dependencies(${Name} IR_INC)
  add_dependencies(${Name} CONFIG_INC)

  target_link_libraries(${Name} pthread rt vixl ${LINUX_LIBS} dl xxhash)
  set_target_properties(${Name} PROPERTIES OUTPUT_NAME FEXCore)
  set_target_properties(${Name} PROPERTIES C_VISIBILITY_PRESET hidden)
  set_target_properties(${Name} PROPERTIES CXX_VISIBILITY_PRESET hidden)
  set_target_properties(${Name} PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)

  target_include_directories(${Name} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

  target_include_directories(${Name} PRIVATE IncludePrivate/)
  target_include_directories(${Name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)

  target_include_directories(${Name} PUBLIC "${PROJECT_SOURCE_DIR}/include/")
  target_include_directories(${Name} PUBLIC "${CMAKE_BINARY_DIR}/include/")

  target_compile_definitions(${Name} PRIVATE ${DEFINES})

  target_compile_options(${Name}
    PRIVATE
    -Wall
    -Werror=implicit-fallthrough

    -Wno-trigraphs
    -ffunction-sections
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
endfunction()

function(AddLibrary Name Type)
  add_library(${Name} ${Type} $<TARGET_OBJECTS:${PROJECT_NAME}_object>)
  target_link_libraries(${Name} pthread rt vixl ${LINUX_LIBS} dl xxhash)
  set_target_properties(${Name} PROPERTIES OUTPUT_NAME FEXCore)
  set_target_properties(${Name} PROPERTIES C_VISIBILITY_PRESET hidden)
  set_target_properties(${Name} PROPERTIES CXX_VISIBILITY_PRESET hidden)
  set_target_properties(${Name} PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)

  target_include_directories(${Name} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
  target_include_directories(${Name} PUBLIC "${PROJECT_SOURCE_DIR}/include/")
  target_include_directories(${Name} PUBLIC "${CMAKE_BINARY_DIR}/include/")

  if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
    target_link_options(${Name}
      PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed"
    )
  endif()
endfunction()

AddObject(${PROJECT_NAME}_object OBJECT)
AddLibrary(${PROJECT_NAME} STATIC)
AddLibrary(${PROJECT_NAME}_shared SHARED)

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_shared
  LIBRARY
    DESTINATION lib
    COMPONENT Libraries
  ARCHIVE
    DESTINATION lib
    COMPONENT Libraries)
