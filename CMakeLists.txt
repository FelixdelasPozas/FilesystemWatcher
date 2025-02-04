cmake_minimum_required (VERSION 3.10)

project (FilesystemWatcher)

# Version Number
set (FILESYSTEM_WATCHER_VERSION_MAJOR 1)
set (FILESYSTEM_WATCHER_VERSION_MINOR 2)
set (FILESYSTEM_WATCHER_VERSION_PATCH 0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories (${CMAKE_SOURCE_DIR})
include_directories (${PROJECT_BINARY_DIR})

# Fixed, need to be changed to your own installation of Logitech Gaming LED SKD files. 
set(LOGITECH_INCLUDE "D:/Desarrollo/Code/LogitechG810/include/")
set(LOGITECH_LIBRARY "D:/Desarrollo/Code/LogitechG810/include/LogitechLedEnginesWrapper.a")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt6 COMPONENTS Core Widgets Multimedia)

include_directories( 
  ${LOGITECH_INCLUDE}
)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

set(CMAKE_CXX_FLAGS " -Wall -mwindows -Wno-deprecated -std=c++17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt6Multimedia_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt6Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt6Core_EXECUTABLE_COMPILE_FLAGS}")

configure_file("${PROJECT_SOURCE_DIR}/FilesystemWatcher.rc.in" "${PROJECT_BINARY_DIR}/FilesystemWatcher.rc")
configure_file("${PROJECT_SOURCE_DIR}/installer/script.iss.in" "${PROJECT_BINARY_DIR}/script.iss")

set (CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
ENABLE_LANGUAGE(RC)

# Add Qt Resource files
qt6_add_resources(RESOURCES
	rsc/resources.qrc
	)

# Create Interface files
qt6_wrap_ui (UI_FILES
	FilesystemWatcher.ui
	AboutDialog.ui
	AddObjectDialog.ui
	)
	
set (SOURCES 
	${RESOURCES}
	${UI_FILES}
	${PROJECT_BINARY_DIR}/FilesystemWatcher.rc
	main.cpp
	FilesystemWatcher.cpp
	AboutDialog.cpp
	AddObjectDialog.cpp
	WatchThread.cpp
	ObjectsTableModel.cpp
	LogiLED.cpp
	Utils.cpp
	)
  
set (EXTERNAL_LIBRARIES
	Qt6::Core
	Qt6::Widgets
	Qt6::Multimedia
	Shlwapi
	${LOGITECH_LIBRARY}
# gcc 13
#	stdc++fs
	)
  
add_executable(FilesystemWatcher ${SOURCES})
target_link_libraries (FilesystemWatcher ${EXTERNAL_LIBRARIES})	
