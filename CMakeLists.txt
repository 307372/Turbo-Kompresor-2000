cmake_minimum_required(VERSION 3.5)

project(Turbo-Kompresor-2000)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
find_library( divsufsort_lib divsufsort )

add_executable(Turbo-Kompresor-2000
  main.cpp
  mainwindow.cpp
  mainwindow.h
  mainwindow.ui
  main.cpp
  statistical_tools.cpp
  statistical_tools.h
  integrity_validation.cpp
  integrity_validation.h
  archive.h
  archive.cpp
  notcompression.h
  notcompression.cpp
  archive_structures.h
  archive_structures.cpp
  project_exceptions.h
  project_exceptions.cpp
  compression.h
  compression.cpp
  resources.qrc
)

target_link_libraries(Turbo-Kompresor-2000 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets "${divsufsort_lib}" )

