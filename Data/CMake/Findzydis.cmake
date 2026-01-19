# SPDX-License-Identifier: MIT

if (CMAKE_CROSSCOMPILING)
    return()
endif()

include(FindPackageHandleStandardArgs)

find_package(zydis QUIET CONFIG)

if (zydis_CONSIDERED_CONFIGS)
    find_package_handle_standard_args(zydis CONFIG_MODE)
else()
    find_package(PkgConfig QUIET)
    pkg_search_module(zydis QUIET IMPORTED_TARGET zydis)
    find_package_handle_standard_args(zydis
        REQUIRED_VARS zydis_LINK_LIBRARIES
        VERSION_VAR zydis_VERSION)

    if (TARGET PkgConfig::zydis)
      add_library(Zydis::Zydis ALIAS PkgConfig::zydis)
    endif()
endif()
