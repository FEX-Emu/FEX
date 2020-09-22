cmake_minimum_required(VERSION 3.14)
project(host-thunks)

set(CMAKE_CXX_STANDARD 17)

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

function(add_host_lib NAME)
    add_library(${NAME}-host SHARED ../lib${NAME}/lib${NAME}_Host.cpp ${GEN_lib${NAME}})
    target_include_directories(${NAME}-host PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen/lib${NAME}" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
endfunction()

generate(libasound function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(asound)

generate(libEGL function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(EGL)

generate(libGL function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(GL)
target_link_libraries(GL-host PRIVATE GL)

# disabled for now, headers are platform specific
# find_package(SDL2 REQUIRED)
# generate(libSDL2 function_unpacks tab_function_unpacks ldr ldr_ptrs)
# add_host_lib(SDL2)
# target_include_directories(SDL2-host PRIVATE ${SDL2_INCLUDE_DIRS})

generate(libX11 function_unpacks tab_function_unpacks ldr ldr_ptrs callback_packs callback_structs callback_unpacks_header callback_typedefs)
add_host_lib(X11)

generate(libXext function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(Xext)

generate(libXrender function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(Xrender)

generate(libXfixes function_unpacks tab_function_unpacks ldr ldr_ptrs)
add_host_lib(Xfixes)