set(NAME CommonCore)
set(SRCS
  VMFactory.cpp)

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} FEXCore)
