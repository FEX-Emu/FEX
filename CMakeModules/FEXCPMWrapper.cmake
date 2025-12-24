# This is a basic wrapper around CPM to reduce boilerplate.
# The primary purpose of this is to work with source archives rather than full Git clones,
# which are significantly faster to fetch and take up less overall space.

function(FEXAddPackage)
    set(oneValueArgs
        NAME
        HASH
        REPO
        VERSION
        COMMIT
        TAG
        SOURCE_SUBDIR)

    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(PKG "" "${oneValueArgs}" "${multiValueArgs}"
        "${ARGN}")

    if (NOT DEFINED PKG_NAME)
        message(FATAL_ERROR "[FEXAddPackage] NAME is required")
    endif()

    if (NOT DEFINED PKG_REPO)
        message(FATAL_ERROR "[FEXAddPackage] REPO is required")
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

    set(url ${git_url}/)

    CPMAddPackage(
        NAME ${PKG_NAME}
        URL ${pkg_url}
        URL_HASH ${PKG_HASH}
        CUSTOM_CACHE_KEY ${key}
        FIND_PACKAGE_ARGUMENTS "GLOBAL"

        OPTIONS ${PKG_OPTIONS}
        PATCHES ${PKG_PATCHES}
        EXCLUDE_FROM_ALL ON # TODO: Is this desired?
        SOURCE_SUBDIR ${PKG_SOURCE_SUBDIR})
endfunction()
