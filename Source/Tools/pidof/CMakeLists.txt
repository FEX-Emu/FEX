add_executable(FEXpidof pidof.cpp)

target_link_libraries(FEXpidof
  PRIVATE
  cpp-optparse
  JemallocDummy
  fmt::fmt
  range-v3::range-v3)

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(FEXpidof
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed")
endif()

install(TARGETS FEXpidof
  RUNTIME
    DESTINATION bin
    COMPONENT Runtime)
