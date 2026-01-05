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

# Description json gets installed into root of depot
install(FILES emulator.json
  DESTINATION /
  COMPONENT Runtime)
install(FILES ConfigTemplate.json
  DESTINATION /
  COMPONENT Runtime)
install(FILES toolmanifest.vdf
  DESTINATION /
  COMPONENT Runtime)
