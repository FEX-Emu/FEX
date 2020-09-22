cmake_minimum_required(VERSION 3.14)
project(guest-thunks)

set(CMAKE_CXX_STANDARD 17)

# These get passed in from the main cmake project
set (X86_C_COMPILER "x86_64-linux-gnu-gcc" CACHE STRING "c compiler for compiling x86 guest libs")
set (X86_CXX_COMPILER "x86_64-linux-gnu-g++" CACHE STRING "c++ compiler for compiling x86 guest libs")

set(CMAKE_C_COMPILER "${X86_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${X86_CXX_COMPILER}")

function(generate NAME)
    foreach(WHAT IN LISTS ARGN)
        set(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/gen/${NAME}/${WHAT}.inl")
        set(GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/../Generators/${NAME}.py")
        set(HELPER "${CMAKE_CURRENT_SOURCE_DIR}/../Generators/ThunkHelpers.py")

        add_custom_command(
            OUTPUT "${OUTFILE}"
            DEPENDS "${GENERATOR}"
            DEPENDS "${HELPER}"
            COMMAND "python3" "${GENERATOR}" "${WHAT}" > "${OUTFILE}"
        )

        list(APPEND OUTPUTS "${OUTFILE}")
    endforeach()
    set(GEN_${NAME} ${OUTPUTS} PARENT_SCOPE)
endfunction()

function(add_guest_lib NAME)
    add_library(${NAME}-guest SHARED ../lib${NAME}/lib${NAME}_Guest.cpp ${GEN_lib${NAME}})
    target_include_directories(${NAME}-guest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/lib${NAME}" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
endfunction()

generate(libasound thunks function_packs function_packs_public)
add_guest_lib(asound)

generate(libEGL thunks function_packs function_packs_public)
add_guest_lib(EGL)

generate(libGL thunks function_packs function_packs_public tab_function_packs)
add_guest_lib(GL)

# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(libSDL2 thunks function_packs function_packs_public)
# add_guest_lib(SDL2)
# target_include_directories(SDL2-guest PRIVATE ${SDL2_INCLUDE_DIRS})
# target_link_libraries(SDL2-guest PRIVATE GL)
# target_link_libraries(SDL2-guest PRIVATE dl)

generate(libX11 thunks function_packs function_packs_public callback_structs callback_unpacks_header callback_unpacks_header_init callback_unpacks callback_typedefs)
add_guest_lib(X11)

generate(libXext thunks function_packs function_packs_public)
add_guest_lib(Xext)

generate(libXrender thunks function_packs function_packs_public)
add_guest_lib(Xrender)

generate(libXfixes thunks function_packs function_packs_public)
add_guest_lib(Xfixes)
