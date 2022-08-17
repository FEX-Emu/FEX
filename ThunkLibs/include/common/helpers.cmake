string(TOUPPER ${THUNKS_TARGET} THUNKS_TARGET_UPPER)
string(TOLOWER ${THUNKS_TARGET} THUNKS_TARGET_LOWER)

set(THUNK_LIBRARY_TARGET "${THUNKS_TARGET_UPPER}_THUNK_LIBRARY")

macro(define_target_names)
  set(TARGET_NAME "${NAME}-${THUNKS_TARGET_LOWER}-${GUEST_ARCH}")
  set(TARGET_DEPS "${TARGET_NAME}-deps")
  set(TARGET_GENS "${TARGET_NAME}-gens")
  set(TARGET_INTERFACE "${TARGET_NAME}-interface")

  set(TARGET_NAME "${TARGET_NAME}" PARENT_SCOPE)
  set(TARGET_DEPS "${TARGET_DEPS}" PARENT_SCOPE)
  set(TARGET_GENS "${TARGET_GENS}" PARENT_SCOPE)
  set(TARGET_INTERFACE "${TARGET_INTERFACE}" PARENT_SCOPE)

  set("${NAME}_TARGET_NAME" "${TARGET_NAME}" PARENT_SCOPE)
  set("${NAME}_TARGET_DEPS" "${TARGET_DEPS}" PARENT_SCOPE)
  set("${NAME}_TARGET_GENS" "${TARGET_GENS}" PARENT_SCOPE)
  set("${NAME}_TARGET_INTERFACE" "${TARGET_INTERFACE}" PARENT_SCOPE)
endmacro()

# Syntax: generate(xyz libxyz-interface.cpp generator-targets...)
# This defines two targets and a custom command:
# - custom command: Main build step that runs the thunk generator on the given interface definition
# - ${TARGET_INTERFACE}: Target for IDE integration (making sure libxyz-interface.cpp shows up as a source file in the project tree)
# - ${TARGET_DEPS}: Interface target to read include directories from which are passed to libclang when parsing the interface definition
function(generate NAME SOURCE_FILE)
  define_target_names()

  # Interface target for the user to add include directories
  add_library(${TARGET_DEPS} INTERFACE)
  target_include_directories(${TARGET_DEPS} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  target_compile_definitions(${TARGET_DEPS} INTERFACE ${THUNK_LIBRARY_TARGET})

  # Shorthand for the include directories added after calling this function.
  # This is not evaluated directly, hence directories added after return are still picked up
  set(include_dirs_prop "$<TARGET_PROPERTY:${TARGET_DEPS},INTERFACE_INCLUDE_DIRECTORIES>")
  set(compile_definitions_prop "$<TARGET_PROPERTY:${TARGET_DEPS},INTERFACE_COMPILE_DEFINITIONS>")

  if (${THUNKS_TARGET} EQUAL Host)
    target_link_libraries(${TARGET_DEPS} INTERFACE FEXLoader)

    # Target for IDE integration
    add_library(${TARGET_INTERFACE} EXCLUDE_FROM_ALL ${SOURCE_FILE})
    target_link_libraries(${TARGET_INTERFACE} PRIVATE ${TARGET_DEPS})
  endif()

  
  set(OUTFOLDER "${CMAKE_CURRENT_BINARY_DIR}/gen/${TARGET_NAME}")
  file(MAKE_DIRECTORY "${OUTFOLDER}")

  set(GENERATOR_H "${OUTFOLDER}/gen.h")

  if (NOT GENERATOR_TARGET)
    set(GENERATOR_TARGET ${GENERATOR_EXE})
  endif()

  # this is a nasty hack
  add_custom_command(
    OUTPUT "${GENERATOR_H}"
    DEPENDS "${TARGET_GENS}"
    DEPENDS "${SOURCE_FILE}"
    DEPENDS "${GENERATOR_TARGET}"
    COMMAND "touch" "${GENERATOR_H}"
  )

  add_library(${TARGET_GENS} OBJECT ${SOURCE_FILE} "${GENERATOR_H}")
  target_link_libraries(${TARGET_GENS} PRIVATE ${TARGET_DEPS})
  

  # don't actually compile
  # target_compile_options(${TARGET_GENS} PRIVATE "-fsyntax-only")
  
  target_compile_options(${TARGET_GENS} PRIVATE "-fplugin=${GENERATOR_EXE}")
  target_compile_options(${TARGET_GENS} PRIVATE "-fplugin-arg-fexthunkgen-libname=lib${NAME}")

  # Run thunk generator for each of the given output files
  foreach(WHAT IN LISTS ARGN)
    set(OUTFILE "${OUTFOLDER}/${WHAT}.inl")

    target_compile_options(${TARGET_GENS} PRIVATE "-fplugin-arg-fexthunkgen-${WHAT}=${OUTFILE}.1")
    target_compile_options(${TARGET_GENS} PRIVATE "-fplugin-arg-fexthunkgen-outfile=$<TARGET_OBJECTS:${TARGET_GENS}>")

    add_custom_command(
      OUTPUT "${OUTFILE}"
      DEPENDS "${TARGET_GENS}"
      DEPENDS "${SOURCE_FILE}"
      COMMAND "cp" "${OUTFILE}.1" "${OUTFILE}"
    )

    list(APPEND OUTPUTS "${OUTFILE}")
  endforeach()
  set(GEN_${TARGET_NAME} ${OUTPUTS} PARENT_SCOPE)
endfunction()

function(add_thunk_lib NAME)
  define_target_names()

  set (SOURCE_FILE "../lib${NAME}/lib${NAME}_${THUNKS_TARGET}.cpp")
  get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
  if (NOT EXISTS "${SOURCE_FILE_ABS}")
    set (SOURCE_FILE "../lib${NAME}/${THUNKS_TARGET}.cpp")
    get_filename_component(SOURCE_FILE_ABS "${SOURCE_FILE}" ABSOLUTE)
    if (NOT EXISTS "${SOURCE_FILE_ABS}")
      message (FATAL_ERROR "Thunk source file for ${THUNKS_TARGET} lib ${NAME} doesn't exist!")
    endif()
  endif()

  add_library(${TARGET_NAME} ${TARGET_TYPE} ${SOURCE_FILE} ${GEN_${TARGET_NAME}})

  # add_dependencies(${TARGET_NAME} ${TARGET_GENS})

  target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/${TARGET_NAME}")
  target_compile_definitions(${TARGET_NAME} PRIVATE ${THUNK_LIBRARY_TARGET})
  target_compile_definitions(${TARGET_NAME} PRIVATE "guest_arch_${GUEST_ARCH}")
  target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_DEPS})

  ## Make signed overflow well defined 2's complement overflow
  target_compile_options(${TARGET_NAME} PRIVATE -fwrapv)
  
  # generated files forward-declare functions that need to be implemented manually, so pass --no-undefined to make sure errors are detected at compile-time rather than runtime
  target_link_options(${TARGET_NAME} PRIVATE "LINKER:--no-undefined")

  if (${THUNKS_TARGET} EQUAL Host)
    target_link_libraries(${TARGET_NAME} PRIVATE dl)
  endif()

  if (GENERATE_INSTALL_TARGETS)
    install(TARGETS ${TARGET_NAME} DESTINATION ${HOSTLIBS_DATA_DIRECTORY}/${THUNKS_TARGET}Thunks/)
    add_dependencies(${THUNKS_ARCH_TARGET} ${TARGET_NAME})
  endif()
endfunction()