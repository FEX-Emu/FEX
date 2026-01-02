set(CMAKE_AUTOMOC ON)

add_executable(FEXConfig Main.cpp Main.h)

set(common_qml
  main.qml
  icon.png)

set(dialog_qml
  qt${QT_VERSION_MAJOR}/FileDialog.qml
  qt${QT_VERSION_MAJOR}/FolderDialog.qml
  qt${QT_VERSION_MAJOR}/MessageDialog.qml)

file(COPY
  ${common_qml} ${dialog_qml} qml.qrc
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

qt_add_resources(qml_qrc ${CMAKE_CURRENT_BINARY_DIR}/qml.qrc)
target_sources(FEXConfig PRIVATE ${qml_qrc})

target_link_libraries(FEXConfig PRIVATE
  Common JemallocDummy
  Qt${QT_VERSION_MAJOR}::Qml
  Qt${QT_VERSION_MAJOR}::Quick
  Qt${QT_VERSION_MAJOR}::Widgets)

LinkerGC(FEXConfig)

install(TARGETS FEXConfig RUNTIME
  DESTINATION bin
  COMPONENT Runtime)
