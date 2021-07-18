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
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/binfmts/)
endfunction()

GenBinFmt(FEX-x86.in)
GenBinFmt(FEX-x86_64.in)
