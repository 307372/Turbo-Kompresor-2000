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
find_package(Threads)
# find_package(Qt6 COMPONENTS Concurrent REQUIRED)
find_library( divsufsort_lib divsufsort )

add_executable(Turbo-Kompresor-2000
  main.cpp

  archive_window.cpp archive_window.h archive_window.ui

  processing_dialog.cpp processing_dialog.h processing_dialog.ui

  settings_dialog.cpp settings_dialog.h settings_dialog.ui

  integrity_validation.cpp integrity_validation.h

  archive.h archive.cpp

  archive_structures.h archive_structures.cpp

  misc/project_exceptions.h misc/project_exceptions.cpp

  compression.h compression.cpp

  config.h config.cpp

  misc/bitbuffer.h misc/bitbuffer.cpp

  misc/processing_helpers.h misc/processing_helpers.cpp

  misc/custom_tree_widget_items.h misc/custom_tree_widget_items.cpp

  misc/multithreading.h

  misc/statistics.h

  misc/models.h

  resources/icons/icons.qrc

  resources/breeze/breeze.qrc
)
#[[qdiag]]
target_link_libraries(Turbo-Kompresor-2000 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent "${divsufsort_lib}" Threads::Threads)#  Qt6::Concurrent )

