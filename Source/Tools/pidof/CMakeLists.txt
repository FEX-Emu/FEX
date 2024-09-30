add_executable(FEXpidof pidof.cpp)
target_include_directories(FEXpidof
  PRIVATE
  ${PROJECT_SOURCE_DIR}/Source/Common/cpp-optparse/)

target_link_libraries(FEXpidof
  PRIVATE
  cpp-optparse
  JemallocDummy
  fmt::fmt
)

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(FEXpidof
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed"
  )
endif()

install(TARGETS FEXpidof
  RUNTIME
    DESTINATION bin
    COMPONENT runtime
)
