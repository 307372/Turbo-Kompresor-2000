cmake_minimum_required(VERSION 3.5)

project(Turbo-Kompresor-2000)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Concurrent REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Concurrent REQUIRED)
# find_package(Qt6 COMPONENTS Concurrent REQUIRED)
find_library( divsufsort_lib divsufsort )

add_executable(Turbo-Kompresor-2000
  main.cpp

  mainwindow.cpp mainwindow.h mainwindow.ui

  mydialog.cpp mydialog.h  mydialog.ui

  statistical_tools.cpp statistical_tools.h

  integrity_validation.cpp integrity_validation.h

  archive.h archive.cpp

  archive_structures.h archive_structures.cpp

  project_exceptions.h project_exceptions.cpp

  compression.h compression.cpp

  config.h config.cpp

  resources.qrc
)
#[[qdiag]]
target_link_libraries(Turbo-Kompresor-2000 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent "${divsufsort_lib}")#  Qt6::Concurrent )

