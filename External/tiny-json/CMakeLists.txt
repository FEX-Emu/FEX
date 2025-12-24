set(NAME tiny-json)
set(SRCS tiny-json.c)
add_library(${NAME} STATIC ${SRCS})

target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(${NAME}::${NAME} ALIAS ${NAME})
