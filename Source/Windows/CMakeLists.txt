function(build_implib name)
  set(name_ex ${name}_ex)
  add_custom_target(${name_ex}lib ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib${name_ex}.a)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib${name_ex}.a
    COMMAND ${CMAKE_DLLTOOL} -d ${CMAKE_CURRENT_SOURCE_DIR}/Defs/${name}.def -k -l lib${name_ex}.a
    COMMENT "Building lib${name_ex}.a"
  )

  add_library(${name_ex} SHARED IMPORTED)
  set_property(TARGET ${name_ex} PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_BINARY_DIR}/lib${name_ex}.a)
  add_dependencies(${name_ex} ${name_ex}lib)
endfunction()

# Output DLL patching binary. Needs to be exactly 32-bytes.
add_custom_target(wine_builtin_patch
  COMMAND echo -n 'Wine builtin DLL\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0' > ${CMAKE_BINARY_DIR}/wine_builtin.bin
  BYPRODUCTS ${CMAKE_BINARY_DIR}/wine_builtin.bin)

function(patch_library_wine target)
  add_dependencies(${target} wine_builtin_patch)
  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND dd bs=32 count=1 seek=2 conv=notrunc if=${CMAKE_BINARY_DIR}/wine_builtin.bin of=$<TARGET_FILE:${target}>
  )
endfunction()

build_implib(ntdll)
build_implib(wow64)

add_subdirectory(Common)

if (_M_ARM_64EC)
  add_subdirectory(ARM64EC)
elseif (_M_ARM_64)
  add_subdirectory(WOW64)
endif()
