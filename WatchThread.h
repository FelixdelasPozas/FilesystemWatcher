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
     *
     */
    explicit WatchThread(const std::filesystem::path &object, const unsigned long events, QObject *p = nullptr);

    /** \brief WatchThread class virtual destructor.
     *
     */
    virtual ~WatchThread()
    {};

    /** \brief Aborts the thread.
     *
     */
    void abort();

    /** \brief Object status.
     *
     */
    enum class Event: char { ADDED, REMOVED, MODIFIED, RENAMED_OLD, RENAMED_NEW };

  signals:
    void modified(const std::wstring obj, const WatchThread::Event event);
    void error(const QString message);

  protected:
    virtual void run() override;

  private:
    /** maps the changes with the corresponding event. */
    const std::map<DWORD, Event> eventMapping =
    {
      { FILE_ACTION_ADDED,            Event::ADDED },
      { FILE_ACTION_REMOVED,          Event::REMOVED },
      { FILE_ACTION_MODIFIED,         Event::MODIFIED },
      { FILE_ACTION_RENAMED_OLD_NAME, Event::RENAMED_OLD },
      { FILE_ACTION_RENAMED_NEW_NAME, Event::RENAMED_NEW }
    };

    /** properties to watch on the object. */
    const DWORD watchProperties =
      FILE_NOTIFY_CHANGE_SECURITY    |
      FILE_NOTIFY_CHANGE_CREATION    |
      FILE_NOTIFY_CHANGE_LAST_ACCESS |
      FILE_NOTIFY_CHANGE_LAST_WRITE  |
      FILE_NOTIFY_CHANGE_SIZE        |
      FILE_NOTIFY_CHANGE_ATTRIBUTES  |
      FILE_NOTIFY_CHANGE_DIR_NAME    |
      FILE_NOTIFY_CHANGE_FILE_NAME;

    /** \brief Helper method to get the string of the given Win32 API error.
     * \param[in] errorCode Win32 API error code.
     *
     */
    static QString getLastErrorString(const DWORD errorCode);

    const std::filesystem::path m_object;      /** path of the object to watch.                            */
    const unsigned long         m_events;      /** events to watch.                                        */
    HANDLE                      m_stopHandle;  /** HANDLES to signal the thread to stop.                   */
    bool                        m_isDirectory; /** True if the object is a directory, false if its a file. */
};

#endif // WATCHTHREAD_H_
