function(GenBinFmt Name)
  # Get the filename only component
  get_filename_component(FMT_NAME ${Name} NAME_WE)

  # Configure it
  configure_file(
    ${Name}
    ${CMAKE_BINARY_DIR}/Data/binfmts/${FMT_NAME})

  # Then install the configured binfmt
  install(
    FILES ${CMAKE_BINARY_DIR}/Data/binfmts/${FMT_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/binfmts/
    COMPONENT Runtime)
endfunction()

if (NOT USE_LEGACY_BINFMTMISC)
  configure_file(FEX-x86.conf.in ${CMAKE_BINARY_DIR}/Data/binfmts/FEX-x86.conf)
  configure_file(FEX-x86_64.conf.in ${CMAKE_BINARY_DIR}/Data/binfmts/FEX-x86_64.conf)

  install(
    FILES ${CMAKE_BINARY_DIR}/Data/binfmts/FEX-x86.conf ${CMAKE_BINARY_DIR}/Data/binfmts/FEX-x86_64.conf
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/binfmt.d/
    COMPONENT Runtime)
else()
  GenBinFmt(FEX-x86.in)
  GenBinFmt(FEX-x86_64.in)
endif()
