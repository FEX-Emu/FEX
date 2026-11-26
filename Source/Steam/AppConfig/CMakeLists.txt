file(GLOB CONFIG_SOURCES CONFIGURE_DEPENDS *.json)

# Any application configuration json file gets installed
foreach(CONFIG_SRC ${CONFIG_SOURCES})
  install(FILES ${CONFIG_SRC}
    DESTINATION share/fex-emu/AppConfig/
    COMPONENT Runtime)
endforeach()
