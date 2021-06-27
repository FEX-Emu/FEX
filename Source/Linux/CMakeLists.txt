set (SRCS
  Utils/ELFContainer.cpp
  Utils/ELFSymbolDatabase.cpp
  )

add_library(FEX_Utils OBJECT ${SRCS})
target_link_libraries(FEX_Utils FEXCore)

target_include_directories(FEX_Utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/)
target_include_directories(FEX_Utils PRIVATE ${CMAKE_BINARY_DIR}/generated)
