# Extracts a version from the passed in version string in the form of "<Major>.<Minor>.<Patch>".
# If a part of the version is missing then it gets set as zero.
# Version variables returned in:
# ${Package}_VERSION_MAJOR
# ${Package}_VERSION_MINOR
# ${Package}_VERSION_PATCH
function(version_to_variables VERSION _Package)
  string(REPLACE "." ";" VERSION_LIST "${VERSION}")
  list (LENGTH VERSION_LIST VERSION_LEN)
  if (${VERSION_LEN} GREATER 0)
    list(GET VERSION_LIST 0 VERSION_MAJOR)
    set(${_Package}_VERSION_MAJOR ${VERSION_MAJOR} PARENT_SCOPE)
  else()
    set(${_Package}_VERSION_MAJOR 0 PARENT_SCOPE)
  endif()

  if (${VERSION_LEN} GREATER 1)
    list(GET VERSION_LIST 1 VERSION_MINOR)
    set(${_Package}_VERSION_MINOR ${VERSION_MINOR} PARENT_SCOPE)
  else()
    set(${_Package}_VERSION_MINOR 0 PARENT_SCOPE)
  endif()

  if (${VERSION_LEN} GREATER 2)
    list(GET VERSION_LIST 2 VERSION_PATCH)
    set(${_Package}_VERSION_PATCH ${VERSION_PATCH} PARENT_SCOPE)
  else()
    set(${_Package}_VERSION_PATCH 0 PARENT_SCOPE)
  endif()
endfunction()
