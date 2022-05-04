Filesystem Watcher
==================

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Little utility to watch files and directories for modifications and alarm the user when it happens. It can monitor individual files, directories and complete subtrees.

The alarms can be a text message (or tray icon message if minimized) a sound alarm, or an alarm using the keyboard lights if you have a Logitech RGB keyboard.

If minimized the application will show a tray icon only with an 'eye of Sauron' animation if a file or directory is being watched.  

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

## External dependencies:
The following libraries are required:
* [Qt opensource framework](http://www.qt.io/).
* [Logitech Gaming LED SDK](https://www.logitechg.com/es-es/innovation/developer-lab.html).

# Install

Download and execute the ![latest release](https://github.com/FelixdelasPozas/FilesystemWatcher/releases) installer.

# Screenshots

Simple main dialog. The last column with the alarm color will only appear if a Logitech RGB keyboard is found on the system.

![Main dialog](https://user-images.githubusercontent.com/12167134/163689600-b2b67215-fa84-4dd9-a3c8-27dc30a7fe23.png)

Add object dialog. The option to use keyboard lights as an alarm will only be available if you have a Logitech RGB keyboard.

![Add object dialog](https://user-images.githubusercontent.com/12167134/109077833-0b1a2b80-76fd-11eb-90cf-f80727e7a155.png)

# Repository information
**Version**: 1.1.3

**Status**: finished

**License**: GNU General Public License 3

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |    7          |  295        |    221           |1253  |
| C/C++ Header                 |    6          |  152        |    445           | 309  |
| CMake                        |    1          |   18        |      7           |  57  |
| **Total**                    |   **14**      |  **465**    |   **673**        |**1619**|
