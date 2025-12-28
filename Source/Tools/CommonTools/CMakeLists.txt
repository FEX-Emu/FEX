set(SRCS DummyHandlers.cpp)

if (NOT MINGW)
  list(APPEND SRCS Linux/Utils/ELFContainer.cpp)
endif()

add_library(CommonTools STATIC ${SRCS})
target_link_libraries(CommonTools FEXCore_Base FEXHeaderUtils)
target_include_directories(CommonTools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
