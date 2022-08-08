set(GUEST_ARCH "x86_64")
set_directory_properties(PROPERTIES CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/toolchain-generic.cmake")
include(${CMAKE_CURRENT_LIST_DIR}/toolchain-generic.cmake NO_POLICY_SCOPE)