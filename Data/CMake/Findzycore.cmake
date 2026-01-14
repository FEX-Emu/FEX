# SPDX-License-Identifier: MIT

if (CMAKE_CROSSCOMPILING)
    return()
endif()

include(FindPackageHandleStandardArgs)

find_package(zycore QUIET CONFIG)

if (zycore_CONSIDERED_CONFIGS)
    find_package_handle_standard_args(zycore CONFIG_MODE)
else()
    find_package(PkgConfig QUIET)
    pkg_search_module(zycore QUIET IMPORTED_TARGET zycore)
    find_package_handle_standard_args(zycore
        REQUIRED_VARS zycore_LINK_LIBRARIES
        VERSION_VAR zycore_VERSION)

    if (TARGET PkgConfig::zycore)
      add_library(Zycore::Zycore ALIAS PkgConfig::zycore)
    endif()
endif()
