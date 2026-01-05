add_executable(FEXRootFSFetcher Main.cpp XXFileHash.cpp)
list(APPEND LIBS FEXCore Common JemallocDummy xxHash::xxhash ${PTHREAD_LIB})

LinkerGC(FEXRootFSFetcher)

install(TARGETS FEXRootFSFetcher RUNTIME
  DESTINATION bin
  COMPONENT Runtime)

target_link_libraries(FEXRootFSFetcher PRIVATE ${LIBS})
