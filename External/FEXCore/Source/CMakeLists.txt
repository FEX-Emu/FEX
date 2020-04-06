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
  Interface/Config/Config.cpp
  Interface/Context/Context.cpp
  Interface/Core/BlockCache.cpp
  Interface/Core/BlockSamplingData.cpp
  Interface/Core/Core.cpp
  Interface/Core/CPUID.cpp
  Interface/Core/Frontend.cpp
  Interface/Core/GdbServer.cpp
  Interface/Core/OpcodeDispatcher.cpp
  Interface/Core/X86Tables.cpp
  Interface/Core/X86DebugInfo.cpp
  Interface/Core/Interpreter/InterpreterCore.cpp
  Interface/Core/LLVMJIT/LLVMCore.cpp
  Interface/Core/LLVMJIT/LLVMMemoryManager.cpp
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
  Interface/HLE/x64/Syscalls.cpp
  Interface/HLE/Syscalls/EPoll.cpp
  Interface/HLE/Syscalls/FD.cpp
  Interface/HLE/Syscalls/FS.cpp
  Interface/HLE/Syscalls/Info.cpp
  Interface/HLE/Syscalls/Ioctl.cpp
  Interface/HLE/Syscalls/Memory.cpp
  Interface/HLE/Syscalls/Numa.cpp
  Interface/HLE/Syscalls/SHM.cpp
  Interface/HLE/Syscalls/Sched.cpp
  Interface/HLE/Syscalls/Socket.cpp
  Interface/HLE/Syscalls/Thread.cpp
  Interface/HLE/Syscalls/Time.cpp
  Interface/HLE/Syscalls/Timer.cpp
  Interface/Memory/MemMapper.cpp
  Interface/Memory/SharedMem.cpp
  Interface/IR/IR.cpp
  Interface/IR/PassManager.cpp
  Interface/IR/Passes/ConstProp.cpp
  Interface/IR/Passes/DeadCodeElimination.cpp
  Interface/IR/Passes/DeadContextStoreElimination.cpp
  Interface/IR/Passes/IRCompaction.cpp
  Interface/IR/Passes/IRValidation.cpp
  Interface/IR/Passes/ValueDominanceValidation.cpp
  Interface/IR/Passes/PhiValidation.cpp
  Interface/IR/Passes/RedundantFlagCalculationElimination.cpp
  Interface/IR/Passes/RegisterAllocationPass.cpp
  Interface/IR/Passes/SyscallOptimization.cpp
  )

set (JIT_LIBS )
if (ENABLE_JIT)
  if (_M_X86_64)
    add_definitions(-D_M_X86_64=1)
    if (NOT FORCE_AARCH64)
      list(APPEND SRCS Interface/Core/JIT/x86_64/JIT.cpp)
    endif()
  endif()
  if(_M_ARM_64)
    add_definitions(-D_M_ARM_64=1)
    add_definitions(-DVIXL_INCLUDE_TARGET_AARCH64=1)
    add_definitions(-DVIXL_CODE_BUFFER_MMAP=1)
    list(APPEND SRCS Interface/Core/JIT/Arm64/JIT.cpp)
    list(APPEND JIT_LIBS vixl)
  endif()
endif()

if (ENABLE_JITSYMBOLS)
  add_definitions(-DENABLE_JITSYMBOLS=1)
endif()

# Generate IR include file
set(OUTPUT_NAME "${CMAKE_BINARY_DIR}/include/FEXCore/IR/IRDefines.inc")
set(INPUT_NAME "${CMAKE_CURRENT_SOURCE_DIR}/Interface/IR/IR.json")

add_custom_command(
  OUTPUT "${OUTPUT_NAME}"
  DEPENDS "${INPUT_NAME}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_generator.py"
  COMMAND "python3" "${CMAKE_CURRENT_SOURCE_DIR}/../Scripts/json_ir_generator.py" "${INPUT_NAME}" "${OUTPUT_NAME}"
  )

set_source_files_properties(${OUTPUT_NAME} PROPERTIES
  GENERATED TRUE)

add_custom_target(IR_INC
  DEPENDS "${OUTPUT_NAME}")

# Add in diagnostic colours if the option is available.
# Ninja code generator will kill colours if this isn't here
check_cxx_compiler_flag(-fdiagnostics-color=always GCC_COLOR)
check_cxx_compiler_flag(-fcolor-diagnostics CLANG_COLOR)

set(LINUX_LIBS
  numa)

function(AddLibrary Name Type)
  add_library(${Name} ${Type} ${SRCS})
  add_dependencies(${Name} IR_INC)
  target_link_libraries(${Name} LLVM pthread rt ${JIT_LIBS} ${LINUX_LIBS} dl)
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
