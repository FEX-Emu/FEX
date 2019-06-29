set (SRCS
  SimpleCodeLoader.cpp)

set (ExampleName SimpleCodeLoader)

add_executable(${ExampleName} ${SRCS})
target_link_libraries(${ExampleName} PRIVATE ${PROJECT_NAME} SonicUtils)

set (SRCS
  SimplePrint.cpp)

set (ExampleName SimplePrint)

add_executable(${ExampleName} ${SRCS})
target_link_libraries(${ExampleName} PRIVATE ${PROJECT_NAME} SonicUtils)


