/*
 File: EventsTableModel.cpp
 Created on: 05/02/2021
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
#include "EventsTableModel.h"

// Qt
#include <QClipboard>
#include <QGuiApplication>
#include <QByteArray>

//-----------------------------------------------------------------------------
EventsTableModel::EventsTableModel(QObject *p)
: QAbstractTableModel{p}
{
}

//-----------------------------------------------------------------------------
QModelIndex EventsTableModel::index(int row, int column, const QModelIndex &parent) const
{
  if(parent == QModelIndex())
  {
    return createIndex(row, column, nullptr);
  }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex EventsTableModel::parent(const QModelIndex &child) const
{
  return QModelIndex();
}

//-----------------------------------------------------------------------------
int EventsTableModel::rowCount(const QModelIndex &parent) const
{
  return m_data.size();
}

//-----------------------------------------------------------------------------
int EventsTableModel::columnCount(const QModelIndex &parent) const
{
  return 2;
}

//-----------------------------------------------------------------------------
QVariant EventsTableModel::data(const QModelIndex &index, int role) const
{
  if(index.isValid() && role == Qt::DisplayRole)
  {
    const auto &data = m_data.at(index.row());

    switch (index.column())
    {
      case 0:
        return QString::fromStdWString(data.first);
        break;
      case 1:
        return eventText(data.second);
        break;
      default:
        break;
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant EventsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal && section < columnCount())
  {
    if(section == 0)
      return tr("Object");
    else
      return tr("Event");
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
QString EventsTableModel::eventText(const WatchThread::Event &e) const
{
  switch(e)
  {
    case WatchThread::Event::ADDED:
      return tr("Added");
      break;
    case WatchThread::Event::MODIFIED:
      return tr("Modified");
      break;
    case WatchThread::Event::REMOVED:
      return tr("Removed");
      break;
    case WatchThread::Event::RENAMED_OLD:
      return tr("Renamed (old name)");
      break;
    case WatchThread::Event::RENAMED_NEW:
      return tr("Renamed (new name)");
      break;
    default:
      break;
  }

  return tr("Unknown event");
}

//-----------------------------------------------------------------------------
void EventsTableModel::modification(const std::wstring obj, const WatchThread::Event e)
{
  beginInsertRows(QModelIndex(), m_data.size(), m_data.size());

  m_data.emplace_back(obj,e);

  endInsertRows();
}

//-----------------------------------------------------------------------------
void EventsTableModel::rename(const std::wstring oldName, const std::wstring newName)
{
  beginInsertRows(QModelIndex(), m_data.size(), m_data.size() + 1);

  m_data.emplace_back(oldName, WatchThread::Event::RENAMED_OLD);
  m_data.emplace_back(newName, WatchThread::Event::RENAMED_NEW);

  endInsertRows();
}

//-----------------------------------------------------------------------------
void EventsTableModel::copyEventsToClipboard()
{
  auto clipboard = QGuiApplication::clipboard();
  clipboard->clear();

  QByteArray text;
  auto printEvent = [&text, this](const std::pair<const std::wstring, const WatchThread::Event> &p)
  {
    std::filesystem::path obj{p.first};
    QString objectType;
    if(std::filesystem::exists(obj))
    {
      objectType = std::filesystem::is_directory(obj) ? tr(" directory") : tr(" file");
    }

    switch(p.second)
    {
      case WatchThread::Event::ADDED:
        text.append("Added%1:");
        break;
      case WatchThread::Event::MODIFIED:
        text.append("Modified%1:");
        break;
      case WatchThread::Event::REMOVED:
        text.append("Removed%1:");
        break;
      case WatchThread::Event::RENAMED_OLD:
        text.append("Renamed%1 from ");
        break;
      case WatchThread::Event::RENAMED_NEW:
        text.append("Renamed%1 to ");
        break;
      default:
        break;
    }

    text.append(QString::fromStdWString(p.first));
    text.append('\n');
  };
  std::for_each(m_data.cbegin(), m_data.cend(), printEvent);

  clipboard->setText(text);
}
