add_executable(FEXRootFSFetcher Main.cpp XXFileHash.cpp)
list(APPEND LIBS FEXCore Common JemallocDummy xxHash::xxhash)

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(FEXRootFSFetcher
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed")
endif()

install(TARGETS FEXRootFSFetcher
  RUNTIME
  DESTINATION bin
  COMPONENT Runtime)

target_link_libraries(FEXRootFSFetcher PRIVATE ${LIBS} ${PTHREAD_LIB})
