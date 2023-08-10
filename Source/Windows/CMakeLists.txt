function(build_implib name)
  add_custom_target(${name}lib ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.a)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.a
    COMMAND ${CMAKE_DLLTOOL} -d ${CMAKE_CURRENT_SOURCE_DIR}/Defs/${name}.def -k -l lib${name}.a
    COMMENT "Building lib${name}.a"
  )

  add_library(${name} SHARED IMPORTED)
  set_property(TARGET ${name} PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.a)
  add_dependencies(${name} ${name}lib)
endfunction()

build_implib(ntdll)
build_implib(wow64)
