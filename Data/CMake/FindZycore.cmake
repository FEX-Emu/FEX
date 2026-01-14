# SPDX-License-Identifier: MIT

if (CMAKE_CROSSCOMPILING)
    return()
endif()

include(FindPackageHandleStandardArgs)

find_package(Zycore QUIET CONFIG)

if (Zycore_CONSIDERED_CONFIGS)
    find_package_handle_standard_args(Zycore CONFIG_MODE)
else()
    find_package(PkgConfig QUIET)
    pkg_search_module(Zycore QUIET IMPORTED_TARGET zycore)
    find_package_handle_standard_args(Zycore
        REQUIRED_VARS zycore_LINK_LIBRARIES
        VERSION_VAR zycore_VERSION)

    if (TARGET PkgConfig::zycore)
      add_library(Zycore::Zycore ALIAS PkgConfig::zycore)
    endif()
endif()
