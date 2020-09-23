set(NAME CommonCore)
set(SRCS
  HostFactory.cpp
  VMFactory.cpp)

add_library(${NAME} STATIC ${SRCS})
target_link_libraries(${NAME} FEXCore)
