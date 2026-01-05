add_executable(FEXGetConfig Main.cpp)

list(APPEND LIBS Common JemallocDummy)

LinkerGC(FEXGetConfig)

install(TARGETS FEXGetConfig RUNTIME
  DESTINATION bin
  COMPONENT Runtime)

target_link_libraries(FEXGetConfig PRIVATE ${LIBS})

target_include_directories(FEXGetConfig PRIVATE ${CMAKE_BINARY_DIR}/generated)
