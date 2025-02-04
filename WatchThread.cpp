/*
 File: WatchThread.cpp
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

// Project
#include <WatchThread.h>

// C++
#include <cassert>
#include <winapifamily.h>
#include <shlwapi.h>
#include <windows.h>
#include <fileapi.h>
#include <array>

//-----------------------------------------------------------------------------
WatchThread::WatchThread(const std::filesystem::path &object, const Events events, bool recursive, QObject *p)
: QThread{p}
, m_object(object)
, m_events{events}
, m_stopHandle{0}
, m_isDirectory{std::filesystem::is_directory(object)}
, m_isRename{false}
, m_recursive{recursive}
{
}

//-----------------------------------------------------------------------------
void WatchThread::abort()
{
  SetEvent(m_stopHandle);

  this->deleteLater();
}

//-----------------------------------------------------------------------------
void WatchThread::run()
{
  const auto id = tr("Monitor thread of '%1'").arg(QString::fromStdWString(m_object.wstring()));
  const auto name = (m_isDirectory) ? m_object.wstring() : m_object.parent_path().wstring();

  auto objectHandle = CreateFileW(name.c_str(),
                                  FILE_LIST_DIRECTORY,
                                  FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
                                  nullptr,
                                  OPEN_EXISTING,
                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                  nullptr);

  if (objectHandle == INVALID_HANDLE_VALUE)
  {
    const auto errorString = getLastErrorString(GetLastError());
    emit error(tr("%1: Unable to create object handle. Error: %2").arg(id).arg(errorString));
    return;
  }

  m_stopHandle = CreateEvent(nullptr, true, false, nullptr);
  if (m_stopHandle == INVALID_HANDLE_VALUE)
  {
    const auto errorString = getLastErrorString(GetLastError());
    emit error(tr("%1: Unable to create signal event handle. Error: %2").arg(id).arg(errorString));
    return;
  }

  OVERLAPPED overlapped{0};
  memset(&overlapped, 0, sizeof(OVERLAPPED));

  std::vector<BYTE> buffer(2048, 0);

  bool async_pending = false;
  DWORD bytes_returned = 0;
  std::array<HANDLE, 2> handles = { objectHandle, m_stopHandle };
  while(true)
  {
    const auto result = ReadDirectoryChangesW(objectHandle,
                                              buffer.data(),
                                              static_cast<DWORD>(buffer.size()),
                                              static_cast<WINBOOL>(m_recursive),
                                              watchProperties,
                                              0,
                                              &overlapped,
                                              0);

    if(result == 0)
    {
      const auto errorString = getLastErrorString(GetLastError());
      emit error(tr("%1: Unable to read changes. Error: %2").arg(id).arg(errorString));
      return;
    }

    async_pending = true;

    switch(WaitForMultipleObjects(2, handles.data(), false, INFINITE))
    {
      case WAIT_OBJECT_0:
        {
          if (!GetOverlappedResult(objectHandle, &overlapped, &bytes_returned, true))
          {
            const auto errorString = getLastErrorString(GetLastError());
            emit error(tr("%1: Unable to finish overlapped IO. Error: %2").arg(id).arg(errorString));
            return;
          }

          async_pending = false;

          if (bytes_returned == 0) break;

          auto information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[0]);
          do
          {
            const std::wstring changed_file_w{ information->FileName, information->FileNameLength / sizeof(information->FileName[0]) };
            const Events event = eventMapping.at(information->Action);
            
            if((m_events & event) != Events::NONE)
            {
              if(!processEvent(changed_file_w, event))
              {
                // break;
                //
                // NOTE: gcc 9.2.0 worked fine, upgrading to gcc 11.3.0
                // broke this giving a modified event for the desired file
                // even when nothing has changed after another HANDLE has been
                // modified in the same directory.
                // Updated: 31-12-2023 Still broken with gcc 13.1.0.
              }
            }

            if (information->NextEntryOffset == 0) break;

            information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(information) + information->NextEntryOffset);
          } while (true);
        }
        break;
      case WAIT_OBJECT_0 + 1:
      // no break;
      default:
        return;
    }
  }

  if (async_pending)
  {
    //clean up running async io
    CancelIo(objectHandle);
    GetOverlappedResult(objectHandle, &overlapped, &bytes_returned, TRUE);
  }
}

//-----------------------------------------------------------------------------
QString WatchThread::getLastErrorString(const DWORD errorCode)
{
  QString message;

  if(errorCode != 0)
  {
    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    //Copy the error message into a std::string.
    message = QString::fromLocal8Bit(messageBuffer, size);

    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
  }

  return message;
}

//-----------------------------------------------------------------------------
bool WatchThread::processEvent(const std::wstring &name, const Events &e)
{
  if(std::filesystem::is_directory(m_object))
  {
    switch(e)
    {
      case Events::RENAMED_NEW:
        emit renamed(m_oldName, m_object.wstring() + L"\\" + name);
        break;
      case Events::RENAMED_OLD:
        m_oldName = m_object.wstring() + L"\\" + name;
        break;
      case Events::NONE:
        return false;
        break;
      default:
        emit modified(m_object.wstring() + L"\\" + name, e);
        break;
    }

    return true;
  }
  else
  {
    auto filename = m_object.filename().wstring();
    std::for_each(filename.begin(), filename.end(), std::towlower);
    auto changedFilename = name;
    std::for_each(changedFilename.begin(), changedFilename.end(), std::towlower);

    if(changedFilename.compare(filename) == 0 || m_isRename)
    {
      switch(e)
      {
        case Events::RENAMED_NEW:
          if(m_isRename)
          {
            const auto oldFilename = m_object.wstring();
            m_object = std::filesystem::path{m_object.parent_path().wstring() + L"\\" + name};
            emit renamed(oldFilename, m_object.wstring());
            m_isRename = false;
          }
          break;
        case Events::RENAMED_OLD:
          // update m_object with the new name the next event.
          m_isRename = true;
          break;
        case Events::NONE:
          return false;
          break;
        default:
          emit modified(m_object.wstring(), e);
          break;
      }

      return true;
    }
  }

  return false;
}
