add_subdirectory(cpp-optparse/)

set(NAME Common)
set(SRCS
  Config.cpp
  CPUInfo.cpp
  ArgumentLoader.cpp
  HostFeatures.cpp
  JSONPool.cpp
  SHMStats.cpp
  VolatileMetadata.cpp
  )

if (NOT MINGW_BUILD)
  list (APPEND SRCS
    FEXServerClient.cpp
    FileFormatCheck.cpp)
endif()

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} FEXCore_Base cpp-optparse tiny-json FEXHeaderUtils range-v3::range-v3)
target_include_directories(${NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)

set_target_properties(${NAME} PROPERTIES
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN TRUE
)
