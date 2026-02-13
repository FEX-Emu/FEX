set(LIBS FEXCore Common CommonTools JemallocLibs)

add_executable(FEXCompatTool CompatTool.cpp)

target_link_libraries(FEXCompatTool PRIVATE ${LIBS})

install(TARGETS FEXCompatTool RUNTIME
  DESTINATION /
  COMPONENT Runtime)

add_executable(FEXServerManager ServerManager.cpp)

target_link_libraries(FEXServerManager PRIVATE ${LIBS})

install(TARGETS FEXServerManager RUNTIME
  DESTINATION bin
  COMPONENT Runtime)

# Description json and VERSION files are installed into the depot root
configure_file(VERSIONS.txt.in ${CMAKE_CURRENT_BINARY_DIR}/VERSIONS.txt)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/VERSIONS.txt
    ConfigTemplate.json
    emulator.json
    toolmanifest.vdf
  DESTINATION /
  COMPONENT Runtime)
