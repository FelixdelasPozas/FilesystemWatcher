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

If you want to support this project you can do it on [Ko-fi](https://ko-fi.com/felixdelaspozas).

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

## External dependencies:
The following libraries are required:
* [Qt Library](http://www.qt.io/).
* [Logitech Gaming LED SDK](https://www.logitechg.com/es-es/innovation/developer-lab.html).

# Install

FileSystemWatcher is available for Windows 10 onwards. You can download the latest installer from the ![releases page](https://github.com/FelixdelasPozas/FilesystemWatcher/releases). Neither the application or the installer are digitally signed so the system will ask for approval before running it the first time.

The last version compatible with Windows 7 & 8 is version 1.1.12 that you can download [here](https://github.com/FelixdelasPozas/FilesystemWatcher/releases/tag/1.1.12).

# Screenshots

Simple main dialog. A last column with the alarm color will only appear if a Logitech RGB keyboard is found on the system.

![Maindialog](https://github.com/user-attachments/assets/c656c101-b44e-498f-905a-b0f2f0d45506)

Add object dialog. The option to use keyboard lights as an alarm will only be available if you have a Logitech RGB keyboard.

![Add object dialog](https://github.com/user-attachments/assets/89fcfe6b-135d-4ee5-af31-d1226d59093e)

# Repository information
**Version**: 1.2.1

**Status**: finished

**License**: GNU General Public License 3

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |    8          |  332        |    240           |1350  |
| C/C++ Header                 |    7          |  167        |    497           | 346  |
| CMake                        |    1          |   18        |      8           |  60  |
| **Total**                    |   **16**      |  **517**    |   **745**        |**1756**|
