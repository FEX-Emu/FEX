cmake_minimum_required(VERSION 3.10)
set(NAME SonicUtils)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRCS
  Source/ELFLoader.cpp
  Source/ELFSymbolDatabase.cpp
  Source/LogManager.cpp)

if (_M_X86_64)
  list(APPEND SRCS Source/VM.cpp)
else()
  list(APPEND SRCS Source/VM_Noop.cpp)
endif()

add_library(${NAME} STATIC ${SRCS})
target_include_directories(${NAME} PUBLIC include/)
