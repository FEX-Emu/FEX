# SPDX-License-Identifier: MIT

#[[
    This is a basic wrapper around CPM to reduce boilerplate.
    The primary purpose of this is to work with source archives rather than full Git clones,
    which are significantly faster to fetch and take up less overall space.

    This module is loosely based on CPMUtil, which can be found at https://git.crueter.xyz/CMake/CPMUtil.
    FEX and its derivatives are granted an exclusive exception to the terms of the LGPLv3, and may use,
    reference, and adapt any code from CPMUtil at will.
]]

set(CPM_SOURCE_CACHE ${PROJECT_SOURCE_DIR}/.cache/cpm)

option(FEX_FORCE_BUNDLED "Force the usage of bundled dependencies" OFF)

include(CPM)
include(Propagate)

function(FEXAddPackage)
    set(oneValueArgs
        NAME
        HASH
        REPO
        VERSION
        COMMIT
        TAG
        SOURCE_SUBDIR
        BUNDLED)

    set(multiValueArgs OPTIONS)

    set(options )

    cmake_parse_arguments(PKG "${options}" "${oneValueArgs}" "${multiValueArgs}"
        "${ARGN}")

    if (NOT DEFINED PKG_NAME)
        message(FATAL_ERROR "[FEXAddPackage] NAME is required")
    endif()

    if (NOT DEFINED PKG_REPO)
        message(FATAL_ERROR "[FEXAddPackage] REPO is required")
    endif()

    if (DEFINED PKG_VERSION)
        set(_version ${PKG_VERSION})
    else()
        set(_version 0)
    endif()

    set(${PKG_NAME}_CUSTOM_DIR "" CACHE STRING
        "Path to a separately-downloaded copy of ${PKG_NAME}")

    # Packagers can explicitly request a custom directory containing
    # a previously-downloaded copy of the repository. This is somewhat similar
    # to prefetching submodules.
    if (DEFINED ${PKG_NAME}_CUSTOM_DIR AND
        NOT ${PKG_NAME}_CUSTOM_DIR STREQUAL "")
        set(CPM_${PKG_NAME}_SOURCE ${${PKG_NAME}_CUSTOM_DIR})
    endif()

    set(git_url https://github.com/${PKG_REPO})

    if (DEFINED PKG_TAG)
        set(key ${PKG_TAG})
        set(pkg_url
            ${git_url}/archive/refs/tags/${PKG_TAG}.tar.gz)
    elseif (DEFINED PKG_COMMIT)
        string(SUBSTRING ${PKG_COMMIT} 0 4 key)
        set(pkg_url
            ${git_url}/archive/${PKG_COMMIT}.tar.gz)
    else()
        message(FATAL_ERROR "[FEXAddPackage] One of TAG or COMMIT is required")
    endif()

    message(STATUS "${pkg_url}")

    if ((DEFINED PKG_BUNDLED AND PKG_BUNDLED) OR FEX_FORCE_BUNDLED)
        set(CPM_USE_LOCAL_PACKAGES OFF)
    else()
        set(CPM_USE_LOCAL_PACKAGES ON)
    endif()

    CPMAddPackage(
        NAME ${PKG_NAME}
        URL ${pkg_url}
        URL_HASH ${PKG_HASH}
        VERSION ${_version}
        CUSTOM_CACHE_KEY ${key}

        OPTIONS ${PKG_OPTIONS}
        PATCHES ${PKG_PATCHES}
        EXCLUDE_FROM_ALL ON # TODO: Is this desired?
        SOURCE_SUBDIR ${PKG_SOURCE_SUBDIR})

    Propagate(${PKG_NAME}_ADDED)
    Propagate(${PKG_NAME}_SOURCE_DIR)
endfunction()
