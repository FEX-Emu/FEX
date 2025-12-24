set(NAME CommonTools)
set(SRCS
  DummyHandlers.cpp
  )

if (NOT MINGW)
  list(APPEND SRCS
  Linux/Utils/ELFContainer.cpp
  )
endif()

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} FEXCore_Base FEXHeaderUtils)
target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
