set(NAME SonicUtils)
set(SRCS
  ELFLoader.cpp
  ELFSymbolDatabase.cpp
  LogManager.cpp
  VM.cpp)
add_library(${NAME} ${SRCS})
