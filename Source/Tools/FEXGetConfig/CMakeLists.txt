add_executable(FEXGetConfig Main.cpp)

list(APPEND LIBS Common JemallocDummy)

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(FEXGetConfig
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed")
endif()

install(TARGETS FEXGetConfig
  RUNTIME
  DESTINATION bin
  COMPONENT Runtime)

target_link_libraries(FEXGetConfig PRIVATE ${LIBS})

target_include_directories(FEXGetConfig PRIVATE ${CMAKE_BINARY_DIR}/generated)
