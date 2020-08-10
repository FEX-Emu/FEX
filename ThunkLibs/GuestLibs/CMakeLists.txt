cmake_minimum_required(VERSION 3.14)
project(guest-thunks)

set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++)

function(generate NAME)
    foreach(WHAT IN LISTS ARGN)
        set(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/gen/${NAME}_${WHAT}.inl")
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
    target_include_directories(${NAME}-guest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen" "${CMAKE_CURRENT_SOURCE_DIR}/../common")
endfunction()

generate(libasound thunks)
add_guest_lib(asound)

generate(libEGL thunks)
add_guest_lib(EGL)

generate(libGL thunks symtab)
add_guest_lib(GL)

find_package(SDL2 REQUIRED)
generate(libSDL2 thunks)
add_guest_lib(SDL2)
target_include_directories(SDL2-guest PRIVATE ${SDL2_INCLUDE_DIRS})

generate(libX11 thunks)
add_guest_lib(X11)

generate(libXext thunks)
add_guest_lib(Xext)

generate(libXrender thunks)
add_guest_lib(Xrender)

generate(libXfixes thunks)
add_guest_lib(Xfixes)