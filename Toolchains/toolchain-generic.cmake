# This is tuned for Ubuntu multiarch / multilib + clang + lld

if ("${GUEST_ARCH}" STREQUAL "x86_32")
    set(CMAKE_SYSTEM_PROCESSOR i386)

    # Fun Fact: Ubuntu's i386 actually uses the i686 compiler
    set(triple i386-linux-gnu)
    set(sysroot_triple i686-linux-gnu)
elseif ("${GUEST_ARCH}" STREQUAL "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)

    # the triple identifies the target platform
    set(triple x86_64-linux-gnu)
    set(sysroot_triple x86_64-linux-gnu)
else()
    message(FATAL_ERROR "Toolchain doesn't support guest arch: |${GUEST_ARCH}|")
endif()

# Force re-checking of the compiler - cmake sometimes gets confused here and skips it
set(CMAKE_C_ABI_COMPILED False)
set(CMAKE_CXX_ABI_COMPILED False)


# By default we use the clang & lld that was used to compile and link FEX, in cross compiler mode
#
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
# we need to use lld for cross compiling
add_link_options("-fuse-ld=lld")


# gcc alternative
#
## set(CMAKE_C_COMPILER i686-linux-gnu-gcc)
## set(CMAKE_C_FLAGS -m32)
## set(CMAKE_CXX_COMPILER i686-linux-gnu-g++)
## set(CMAKE_CXX_FLAGS -m32)

set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

# Set the compiler to looks for libs, includes and packages in the correct folders
# as seen on the official cmake site
set(CMAKE_SYSROOT_COMPILE "/usr/${sysroot_triple}")
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "/usr/include")
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_C_STANDARD_INCLUDE_DIRECTORIES})

set(CMAKE_SYSROOT_LINK "/lib/${sysroot_triple}")
add_link_options("-L/usr/${sysroot_triple}/lib" "-L/usr/lib/${sysroot_triple}")

set(CMAKE_FIND_ROOT_PATH "/usr/${sysroot_triple}")


set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)