set(CMAKE_AUTOMOC ON)

add_executable(FEXConfig)
target_sources(FEXConfig PRIVATE Main.cpp Main.h)
target_include_directories(FEXConfig PRIVATE ${CMAKE_SOURCE_DIR}/Source/)
target_link_libraries(FEXConfig PRIVATE Common JemallocDummy)
if (Qt6_FOUND)
  qt_add_resources(QT_RESOURCES qml6.qrc)
  target_link_libraries(FEXConfig PRIVATE Qt6::Qml Qt6::Quick Qt6::Widgets)
else()
  qt_add_resources(QT_RESOURCES qml5.qrc)
  target_link_libraries(FEXConfig PRIVATE Qt5::Qml Qt5::Quick Qt5::Widgets)
endif()
target_sources(FEXConfig PRIVATE ${QT_RESOURCES})

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
  target_link_options(FEXConfig
    PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed"
  )
endif()

install(TARGETS FEXConfig
  RUNTIME
  DESTINATION bin
  COMPONENT Runtime)
