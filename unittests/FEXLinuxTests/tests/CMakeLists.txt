cmake_minimum_required(VERSION 3.14)
project(FEXLinuxTests)

set(CMAKE_CXX_STANDARD 17)

unset (CMAKE_C_FLAGS)
unset (CMAKE_CXX_FLAGS)

set(GENERATE_GUEST_INSTALL_TARGETS TRUE)

file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS *.cpp)

foreach(TEST ${TESTS})
  get_filename_component(TEST_NAME ${TEST} NAME_WLE)

  # Used to insert a configuration dependency to the test file
  CONFIGURE_FILE(${TEST} ${CMAKE_BINARY_DIR}/junk.file)

  file(READ ${TEST} TEST_CODE)
  set(FLAGS_REGEX "//[ ]*append cxxflags: ([^\n]+)")

  string(REGEX MATCH ${FLAGS_REGEX} APPEND_CXX_FLAGS ${TEST_CODE})
  # if cannot handle multiline variables, so we have to match the line first
  if(${APPEND_CXX_FLAGS} MATCHES ${FLAGS_REGEX})
    set(APPEND_CXX_FLAGS "${CMAKE_MATCH_1}")
  else()
    set(APPEND_CXX_FLAGS "")
  endif()

  set(FLAGS_REGEX "//[ ]*append ldflags: ([^\n]+)")

  string(REGEX MATCH ${FLAGS_REGEX} APPEND_LD_FLAGS ${TEST_CODE})
  # if cannot handle multiline variables, so we have to match the line first
  if(${APPEND_LD_FLAGS} MATCHES ${FLAGS_REGEX})
    set(APPEND_LD_FLAGS "${CMAKE_MATCH_1}" )
  else()
    set(APPEND_LD_FLAGS "")
  endif()

  set(LIBS_REGEX "//[ ]*libs: ([^\n]+)")

  string(REGEX MATCH ${LIBS_REGEX} LIBS ${TEST_CODE})
  # if cannot handle multiline variables, so we have to match the line first
  if(${LIBS} MATCHES ${LIBS_REGEX})
    set(LIBS "${CMAKE_MATCH_1}" )
    string(REGEX REPLACE " |," ";" LIBS "${LIBS}")
  else()
    set(LIBS "")
  endif()

  set(ARCH_REGEX "//[ ]*arch: ([^\n]+)")

  string(REGEX MATCH ${ARCH_REGEX} ARCHES ${TEST_CODE})
  # if cannot handle multiline variables, so we have to match the line first
  if(${ARCHES} MATCHES ${ARCH_REGEX})
    set(ARCHES "${CMAKE_MATCH_1}" )
    string(REGEX REPLACE " |," ";" ARCHES "${ARCHES}")
  else()
    set(ARCHES "${GUEST_ARCH}" "")
  endif()

  # message("test: ${TEST_NAME} |${GUEST_ARCH}| in [${ARCHES}]")

  if (${GUEST_ARCH} IN_LIST ARCHES)
    set(BIN_NAME "${TEST_NAME}.${GUEST_ARCH}")

    # message("Added Target ${BIN_NAME}")

    add_executable(${BIN_NAME} ${TEST})
    set_target_properties(${BIN_NAME} PROPERTIES COMPILE_FLAGS "${APPEND_CXX_FLAGS} -g -O2" LINK_FLAGS "${APPEND_LD_FLAGS}")
    target_link_libraries(${BIN_NAME} ${LIBS})
  endif()

endforeach()
