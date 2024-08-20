set(CMAKE_AUTOMOC ON)
qt_add_resources(QT_RESOURCES qml.qrc)

add_executable(FEXQonfig)
target_sources(FEXQonfig PRIVATE Main.cpp ConfigModel.h ${QT_RESOURCES})
target_include_directories(FEXQonfig PRIVATE ${CMAKE_SOURCE_DIR}/Source/)
target_link_libraries(FEXQonfig PRIVATE Common)
target_link_libraries(FEXQonfig PRIVATE Qt5::Qml Qt5::Quick Qt5::Widgets)

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(FEXQonfig
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed"
  )
endif()

install(TARGETS FEXQonfig
  RUNTIME
  DESTINATION bin
  COMPONENT runtime)
