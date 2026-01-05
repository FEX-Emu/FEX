add_executable(FEXBash FEXBash.cpp)

target_link_libraries(FEXBash PRIVATE FEXCore Common JemallocLibs)

LinkerGC(FEXBash)

install(TARGETS FEXBash RUNTIME
  DESTINATION bin
  COMPONENT Runtime)
