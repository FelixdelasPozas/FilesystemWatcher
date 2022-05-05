project (FilesystemWatcher)

cmake_minimum_required (VERSION 3.21)

# Version Number
set (FILESYSTEM_WATCHER_VERSION_MAJOR 1)
set (FILESYSTEM_WATCHER_VERSION_MINOR 1)
set (FILESYSTEM_WATCHER_VERSION_PATCH 3)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)

string(TIMESTAMP CURRENT_YEAR "%Y")

include_directories (${CMAKE_SOURCE_DIR})
include_directories (${PROJECT_BINARY_DIR})

# Fixed, need to be changed to your own installation of Logitech Gaming LED SKD files. 
set(LOGITECH_INCLUDE "D:/Desarrollo/Code/LogitechG810/include/")
set(LOGITECH_LIBRARY "D:/Desarrollo/Code/LogitechG810/include/LogitechLedEnginesWrapper.a")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Multimedia)

include_directories ( ${Qt5Widgets_INCLUDE_DIRS}
                      ${Qt5Core_INCLUDE_DIRS}
                      ${LOGITECH_INCLUDE})

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

set(CMAKE_CXX_FLAGS " -Wall -mwindows -Wno-deprecated -std=c++17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Multimedia_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

configure_file("${PROJECT_SOURCE_DIR}/FilesystemWatcher.rc.in" "${PROJECT_BINARY_DIR}/FilesystemWatcher.rc")
configure_file("${PROJECT_SOURCE_DIR}/installer/script.iss.in" "${PROJECT_BINARY_DIR}/script.iss")

set (CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
ENABLE_LANGUAGE(RC)

# Add Qt Resource files
qt5_add_resources(RESOURCES
	rsc/resources.qrc
	)

# Create Interface files
qt5_wrap_ui (UI_FILES
	FilesystemWatcher.ui
	AboutDialog.ui
	AddObjectDialog.ui
	)
	
set (SOURCES 
	${RESOURCES}
	${PROJECT_BINARY_DIR}/FilesystemWatcher.rc
	main.cpp
	FilesystemWatcher.cpp
	AboutDialog.cpp
	AddObjectDialog.cpp
	WatchThread.cpp
	ObjectsTableModel.cpp
	LogiLED.cpp
	)
  
set (EXTERNAL_LIBRARIES
	Qt5::Core
	Qt5::Widgets
	Qt5::Multimedia
	Shlwapi
	${LOGITECH_LIBRARY}
	)
  
add_executable(FilesystemWatcher ${SOURCES})
target_link_libraries (FilesystemWatcher ${EXTERNAL_LIBRARIES})	
