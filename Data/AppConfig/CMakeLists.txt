file(GLOB CONFIG_SOURCES CONFIGURE_DEPENDS *.json)
file(GLOB GEN_CONFIG_SOURCES CONFIGURE_DEPENDS *.json.in)

# Any application configuration json file gets installed
foreach(CONFIG_SRC ${CONFIG_SOURCES})
  install(FILES ${CONFIG_SRC}
    DESTINATION ${DATA_DIRECTORY}/AppConfig/)
endforeach()

# Any configuration file json file that needs to be generated
# First generate then install it
foreach(GEN_CONFIG_SRC ${GEN_CONFIG_SOURCES})
  # Get the filename only component
  get_filename_component(CONFIG_NAME ${GEN_CONFIG_SRC} NAME_WE)

  # Configure it
  configure_file(
    ${GEN_CONFIG_SRC}
    ${CMAKE_BINARY_DIR}/Data/AppConfig/${CONFIG_NAME}.json)

  # Then install the configured json
  install(
    FILES ${CMAKE_BINARY_DIR}/Data/AppConfig/${CONFIG_NAME}.json
    DESTINATION ${DATA_DIRECTORY}/AppConfig/)
endforeach()
