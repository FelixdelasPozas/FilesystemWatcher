/*
 File: WatchThread.h
 Created on: 08/02/2021
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WATCHTHREAD_H_
#define WATCHTHREAD_H_

// Qt
#include <QThread>

// C++
#include <filesystem>
#include <minwindef.h>
#include <synchapi.h>
#include <vector>
#include <map>
#include <type_traits>

enum class Events: char
{
  NONE        = 0,
  ADDED       = 0b00000001,
  REMOVED     = 0b00000010,
  MODIFIED    = 0b00000100,
  RENAMED_OLD = 0b00001000,
  RENAMED_NEW = 0b00010000,
  RECURSIVE   = 0b00100000 /** added by me for UI reasons, not in the api. */
};

inline Events operator|(Events lhs, Events rhs)
{ return static_cast<Events>(static_cast<std::underlying_type_t<Events>>(lhs)|static_cast<std::underlying_type_t<Events>>(rhs)); }

inline Events operator&(Events lhs, Events rhs)
{ return static_cast<Events>(static_cast<std::underlying_type_t<Events>>(lhs)&static_cast<std::underlying_type_t<Events>>(rhs)); }

inline Events operator|=(Events &lhs, Events rhs)
{ lhs = lhs|rhs; return lhs; }

/** \class WatchThread
 * \brief Thread wathing objects.
 *
 */
class WatchThread
: public QThread
{
    Q_OBJECT
  public:

    /** \brief WatchThread class constructor.
     * \param[in] object Path of the object to watch.
     * \param[in] properties Events to watch.
     * \param[in] p Raw pointer of the object parent of this one.
     * \param[in] recursive True to monitor the directory subtree, false to only monitor the directory files.
     *
     */
    explicit WatchThread(const std::filesystem::path &object, const Events events, bool recursive = false, QObject *p = nullptr);

    /** \brief WatchThread class virtual destructor.
     *
     */
    virtual ~WatchThread()
    {};

    /** \brief Aborts the thread.
     *
     */
    void abort();

  signals:
    void renamed(const std::wstring oldName, const std::wstring newName);
    void modified(const std::wstring obj, const Events event);
    void error(const QString message);

  protected:
    virtual void run() override;

  private:
    /** Maps the changes with the corresponding event.
     *
     *  From https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-file_notify_information                 */
    const std::map<DWORD, Events> eventMapping =
    {
      { FILE_ACTION_ADDED,            Events::ADDED },        /** The file was added to the directory.                  */
      { FILE_ACTION_REMOVED,          Events::REMOVED },      /** The file was removed from the directory.              */
      { FILE_ACTION_MODIFIED,         Events::MODIFIED },     /** The file was modified. This can be a change in the
                                                                 time stamp or attributes.                             */
      { FILE_ACTION_RENAMED_OLD_NAME, Events::RENAMED_OLD },  /** The file was renamed and this is the old name.        */
      { FILE_ACTION_RENAMED_NEW_NAME, Events::RENAMED_NEW }   /** The file was renamed and this is the new name.        */
    };

    /** Properties to watch on a file or directory object. Only last access and creation applies to a directory. The
     * rest applies for both.
     *
     * From https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw
     * Retrieves information that describes the changes within the specified directory. The function does not report
     * changes to the specified directory itself.                                                                      */
    const DWORD watchProperties =
      FILE_NOTIFY_CHANGE_FILE_NAME   |/** Any file name change in the watched directory or subtree causes a change
                                        *  notification wait operation to return. Changes include renaming, creating,
                                        *  or deleting a file.                                                         */
      FILE_NOTIFY_CHANGE_DIR_NAME    | /** Any directory-name change in the watched directory or subtree causes a
                                        *  change notification wait operation to return. Changes include creating or
                                        *  deleting a directory.                                                       */
      FILE_NOTIFY_CHANGE_ATTRIBUTES  | /** Any attribute change in the watched directory or subtree causes a change
                                        *  notification wait operation to return.                                      */
      FILE_NOTIFY_CHANGE_SIZE        | /** Any file-size change in the watched directory or subtree causes a change
                                        *  notification wait operation to return. The operating system detects a change
                                        *  in file size only when the file is written to the disk. For operating systems
                                        *  that use extensive caching, detection occurs only when the cache is
                                        *  sufficiently flushed.                                                       */
      FILE_NOTIFY_CHANGE_LAST_WRITE  | /** Any change to the last write-time of files in the watched directory or
                                        *  subtree causes a change notification wait operation to return. The operating
                                        *  system detects a change to the last write-time only when the file is written
                                        *  to the disk. For operating systems that use extensive caching, detection
                                        *  occurs only when the cache is sufficiently flushed.                         */
      FILE_NOTIFY_CHANGE_LAST_ACCESS | /** Any change to the last access time of files in the watched directory or
                                        *  subtree causes a change notification wait operation to return.              */
      FILE_NOTIFY_CHANGE_CREATION    | /** Any change to the creation time of files in the watched directory or
                                        *  subtree causes a change notification wait operation to return.              */
      FILE_NOTIFY_CHANGE_SECURITY;     /** Any security-descriptor change in the watched directory or subtree causes a
                                        *  change notification wait operation to return.                               */

    /** \brief Helper method to get the string of the given Win32 API error.
     * \param[in] errorCode Win32 API error code.
     *
     */
    static QString getLastErrorString(const DWORD errorCode);

    /** \brief Processes the event for the 'name' object. Returns true on success and
     *  false otherwise. Event is not a composition of flags, just an individual event.
     * \param[in] name Name given in the event information struct.
     * \param[in] e Event.
     *
     */
    bool processEvent(const std::wstring &name, const Events &e);

    std::filesystem::path m_object;      /** path of the object to watch.                            */
    const Events          m_events;      /** events to watch.                                        */
    HANDLE                m_stopHandle;  /** HANDLES to signal the thread to stop.                   */
    bool                  m_isDirectory; /** True if the object is a directory, false if its a file. */
    std::wstring          m_oldName;     /** old name in case of a rename event.                     */
    bool                  m_isRename;    /** True when a rename event is received with the old name
                                             to signal that the next event will rename m_object.     */
    const bool            m_recursive;   /** True to monitor the directory subtree and false to
                                             monitor only the files in the directory.                */
};

#endif // WATCHTHREAD_H_
