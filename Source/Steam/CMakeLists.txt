add_executable(FEXCompatTool
  CompatTool.cpp)

target_link_libraries(FEXCompatTool
  PRIVATE
  FEXCore Common CommonTools JemallocLibs)

install(TARGETS FEXCompatTool
  RUNTIME
    DESTINATION /
    COMPONENT Runtime
)

# Description json gets installed into root of depot
install(FILES emulator.json
  DESTINATION /
  COMPONENT Runtime)
install(FILES ConfigTemplate.json
  DESTINATION /
  COMPONENT Runtime)
