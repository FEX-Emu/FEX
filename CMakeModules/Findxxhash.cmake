# SPDX-License-Identifier: MIT

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
pkg_search_module(xxhash QUIET IMPORTED_TARGET xxhash libxxhash)
find_package_handle_standard_args(xxhash
    REQUIRED_VARS xxhash_LINK_LIBRARIES
    VERSION_VAR xxhash_VERSION
)

if (xxhash_FOUND AND NOT TARGET xxHash::xxhash)
    if (TARGET PkgConfig::xxhash)
        add_library(xxHash::xxhash ALIAS PkgConfig::xxhash)
    else()
        add_library(xxHash::xxhash ALIAS xxhash)
    endif()
endif()
