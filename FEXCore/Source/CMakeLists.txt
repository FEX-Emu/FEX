set (MAN_DIR share/man CACHE PATH "MAN_DIR")

set (FEXCORE_BASE_SRCS
  Interface/Config/Config.cpp
  Utils/Allocator.cpp
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
  Interface/Context/Context.cpp
  Interface/Core/LookupCache.cpp
  Interface/Core/CodeCache.cpp
  Interface/Core/Core.cpp
  Interface/Core/CPUBackend.cpp
  Interface/Core/Addressing.cpp
  Interface/Core/CPUID.cpp
  Interface/Core/Frontend.cpp
  Interface/Core/OpcodeDispatcher/AVX_128.cpp
  Interface/Core/OpcodeDispatcher/Crypto.cpp
  Interface/Core/OpcodeDispatcher/Flags.cpp
  Interface/Core/OpcodeDispatcher/Vector.cpp
  Interface/Core/OpcodeDispatcher/X87.cpp
  Interface/Core/OpcodeDispatcher/X87F64.cpp
  Interface/Core/OpcodeDispatcher.cpp
  Interface/Core/ArchHelpers/Arm64Emitter.cpp
  Interface/Core/Dispatcher/Dispatcher.cpp
  Interface/Core/Interpreter/Fallbacks/InterpreterFallbacks.cpp
  Interface/Core/Interpreter/Fallbacks/StringCompareFallbacks.cpp
  Interface/Core/JIT/JIT.cpp
  Interface/Core/JIT/ALUOps.cpp
  Interface/Core/JIT/AtomicOps.cpp
  Interface/Core/JIT/BranchOps.cpp
  Interface/Core/JIT/ConversionOps.cpp
  Interface/Core/JIT/EncryptionOps.cpp
  Interface/Core/JIT/MemoryOps.cpp
  Interface/Core/JIT/MiscOps.cpp
  Interface/Core/JIT/MoveOps.cpp
  Interface/Core/JIT/VectorOps.cpp
  Interface/Core/JIT/Arm64Relocations.cpp
  Interface/Core/X86Tables/BaseTables.cpp
  Interface/Core/X86Tables/DDDTables.cpp
  Interface/Core/X86Tables/H0F38Tables.cpp
  Interface/Core/X86Tables/H0F3ATables.cpp
  Interface/Core/X86Tables/PrimaryGroupTables.cpp
  Interface/Core/X86Tables/SecondaryGroupTables.cpp
  Interface/Core/X86Tables/SecondaryModRMTables.cpp
  Interface/Core/X86Tables/SecondaryTables.cpp
  Interface/Core/X86Tables/VEXTables.cpp
  Interface/Core/X86Tables/X87Tables.cpp
  Interface/GDBJIT/GDBJIT.cpp
  Interface/IR/IRDumper.cpp
  Interface/IR/IREmitter.cpp
  Interface/IR/PassManager.cpp
  Interface/IR/Passes/IRDumperPass.cpp
  Interface/IR/Passes/IRValidation.cpp
  Interface/IR/Passes/RedundantFlagCalculationElimination.cpp
  Interface/IR/Passes/RegisterAllocationPass.cpp
  Interface/IR/Passes/x87StackOptimizationPass.cpp
  Utils/LongJump.cpp
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

set(DEFINES -DJIT_ARM64)

if (_M_X86_64)
  list(APPEND DEFINES -D_M_X86_64=1)
endif()

if (_M_ARM_64)
  list(APPEND DEFINES -D_M_ARM_64=1)
endif()

if (ENABLE_VIXL_DISASSEMBLER)
  list(APPEND DEFINES -DVIXL_DISASSEMBLER=1)
endif()

if (_M_ARM_64 AND HAS_CLANG_PRESERVE_ALL)
  list(APPEND DEFINES "-DFEXCORE_PRESERVE_ALL_ATTR=__attribute__((preserve_all));-DFEXCORE_HAS_PRESERVE_ALL_ATTR=1")
else()
  list(APPEND DEFINES "-DFEXCORE_PRESERVE_ALL_ATTR=;-DFEXCORE_HAS_PRESERVE_ALL_ATTR=0")
endif()

set (LIBS fmt::fmt xxHash::xxhash FEXHeaderUtils CodeEmitter cephes_128bit)

if (ENABLE_VIXL_DISASSEMBLER OR ENABLE_VIXL_SIMULATOR)
  list (APPEND LIBS vixl)
endif()

if (NOT MINGW_BUILD)
  list (APPEND LIBS dl)
else()
  list (APPEND LIBS synchronization)
  if (_M_ARM_64EC)
    list (APPEND LIBS mincore)
  endif()
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
install(FILES ${OUTPUT_MAN_NAME_COMPRESS} COMPONENT Runtime DESTINATION ${MAN_DIR}/man1)

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

if (ENABLE_FEXCORE_PROFILER AND FEXCORE_PROFILER_BACKEND STREQUAL "TRACY")
  target_link_libraries(FEXCore_Base TracyClient)
endif()

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
  install(TARGETS ${PROJECT_NAME}_shared
    LIBRARY
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT Libraries)
endif()

# Meta-library to link jemalloc libraries enabled in the build configuration.
# Only needed for targets that run emulation. For others, use JemallocDummy.
add_library(JemallocLibs STATIC Utils/AllocatorHooks.cpp)
if (ENABLE_JEMALLOC)
  target_compile_definitions(JemallocLibs PRIVATE ENABLE_JEMALLOC=1 JEMALLOC_NO_RENAME=1)
  target_link_libraries(JemallocLibs PUBLIC FEX_jemalloc)
endif()
if (ENABLE_JEMALLOC_GLIBC_ALLOC)
  set_source_files_properties(Interface/HLE/Thunks/Thunks.cpp PROPERTIES COMPILE_DEFINITIONS ENABLE_JEMALLOC_GLIBC=1)
  target_link_libraries(JemallocLibs INTERFACE FEX_jemalloc_glibc)
endif()

if (NOT MINGW_BUILD)
  # Dummy project to use for host tools.
  # This overrides use of jemalloc in FEXCore with the normal glibc allocator.
  add_library(JemallocDummy STATIC Utils/AllocatorHooks.cpp)
  target_include_directories(JemallocDummy PRIVATE "${PROJECT_SOURCE_DIR}/include/")
endif()

# The shared library should always link enabled jemalloc libraries
target_link_libraries(${PROJECT_NAME}_shared JemallocLibs)
