add_library(FEXGDBReader SHARED FEXGDBReader.cpp)

install(TARGETS FEXGDBReader
  RUNTIME
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gdb
  COMPONENT Development)

target_include_directories(FEXGDBReader PRIVATE ${CMAKE_BINARY_DIR}/generated)

# We don't actually link, but this is a nice way to get the include dirs
target_link_libraries(FEXGDBReader PRIVATE Common)
