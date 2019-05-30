set(NAME SonicUtils)
set(SRCS ELFLoader.cpp LogManager.cpp VM.cpp)
add_library(${NAME} ${SRCS})
