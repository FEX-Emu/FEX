# SPDX-License-Identifier: MIT

if (ENABLE_ZYDIS)
    FEXAddPackage(
        NAME Zycore
        REPO zyantific/zycore-c
        VERSION 1.5
        COMMIT 75a36c45ae)

    set(ZYDIS_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ZYAN_SYSTEM_ZYCORE ON)

    FEXAddPackage(
        NAME Zydis
        REPO zyantific/zydis
        VERSION 4.0
        COMMIT 9bfadd6a55)
endif()
