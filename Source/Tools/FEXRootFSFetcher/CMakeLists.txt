set(NAME FEXRootFSFetcher)
set(SRCS Main.cpp XXFileHash.cpp)

add_executable(${NAME} ${SRCS})
list(APPEND LIBS FEXCore Common JemallocDummy xxHash::xxhash)

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(${NAME}
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed")
endif()

install(TARGETS ${NAME}
  RUNTIME
  DESTINATION bin
  COMPONENT Runtime)

target_link_libraries(${NAME} PRIVATE ${LIBS} ${PTHREAD_LIB})
