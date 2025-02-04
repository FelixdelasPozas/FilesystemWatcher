/*
 File: ObjectsTableModel.cpp
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
#include <ObjectsTableModel.h>
#include <LogiLED.h>

// Qt
#include <QString>
#include <QColor>

//-----------------------------------------------------------------------------
ObjectsTableModel::ObjectsTableModel(QObject *p)
: QAbstractTableModel{p}
{
}

//-----------------------------------------------------------------------------
QModelIndex ObjectsTableModel::index(int row, int column, const QModelIndex &parent) const
{
  if(parent == QModelIndex())
  {
    return createIndex(row, column, nullptr);
  }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex ObjectsTableModel::parent(const QModelIndex &child) const
{
  return QModelIndex();
}

//-----------------------------------------------------------------------------
int ObjectsTableModel::rowCount(const QModelIndex &parent) const
{
  return m_data.size();
}

//-----------------------------------------------------------------------------
int ObjectsTableModel::columnCount(const QModelIndex &parent) const
{
  if(LogiLED::isAvailable()) return 4;
  return 3;
}

//-----------------------------------------------------------------------------
QVariant ObjectsTableModel::data(const QModelIndex &index, int role) const
{
  if(index.isValid())
  {
    switch(role)
    {
      case Qt::DisplayRole:
        switch(index.column())
        {
          case 0:
            return QString::fromStdWString(std::get<0>(m_data.at(index.row())));
            break;
          case 1:
            {
              auto text = std::get<1>(m_data.at(index.row()));
              if(!text.empty()) return QString::fromStdWString(text);
              else              return QString("Unmodified");
            }
            break;
          case 2:
            return QString::number(std::get<2>(m_data.at(index.row())));
          case 3:
            {
              const auto color = std::get<3>(m_data.at(index.row()));
              if(!color.isValid()) return tr("None");
            }
            return tr(" ");
            break;
          default:
            break;
        }
        break;
      case Qt::BackgroundRole:
        switch(index.column())
        {
          case 3:
            {
              const auto color = std::get<3>(m_data.at(index.row()));
              if(color.isValid()) return color;
            }
            break;
          case 1:
            {
              auto changes = std::get<2>(m_data.at(index.row()));
              if(changes > 0) return QColor(200,120,120);
            }
            break;
        }
        break;
      case Qt::TextAlignmentRole:
        switch(index.column())
        {
          case 0:
            break;
          default:
            return Qt::AlignCenter;
        }

      default:
        break;
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant ObjectsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal && section < columnCount())
  {
    switch(section)
    {
      case 0:
        return tr("Object");
        break;
      case 1:
        return tr("Last Event");
        break;
      case 2:
        return tr("Nº of Events");
        break;
      case 3:
        return tr("Color");
      default:
        break;
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
void ObjectsTableModel::modification(const std::wstring obj, const Events e)
{
  auto objText = QString::fromStdWString(obj);

  auto matchObj = [&objText](std::tuple<std::wstring, std::wstring, unsigned long, QColor> &p)
  {
    auto name = QString::fromStdWString(std::get<0>(p));
    return objText.startsWith(name);
  };
  auto it = std::find_if(m_data.begin(), m_data.end(), matchObj);

  if(it != m_data.end())
  {
    auto &data = *it;
    std::get<1>(data) = eventText(e).toStdWString();
    std::get<2>(data) += 1;

    const auto distance = std::distance(m_data.begin(), it);
    auto tl = index(distance, 1);
    auto br = index(distance, 2);

    emit dataChanged(tl, br, { Qt::DisplayRole, Qt::BackgroundRole });
  }
}

//-----------------------------------------------------------------------------
void ObjectsTableModel::rename(const std::wstring oldName, const std::wstring newName)
{
  auto objText = QString::fromStdWString(oldName);

  auto matchObj = [&objText](std::tuple<std::wstring, std::wstring, unsigned long, QColor> &p)
  {
    auto name = QString::fromStdWString(std::get<0>(p));
    return objText.startsWith(name) || name.compare(objText, Qt::CaseInsensitive) == 0;
  };
  auto it = std::find_if(m_data.begin(), m_data.end(), matchObj);

  if(it != m_data.end())
  {
    auto &data = *it;
    std::get<0>(data) = newName;
    std::get<1>(data) = eventText(Events::RENAMED_NEW).toStdWString();
    std::get<2>(data) += 1;

    const auto distance = std::distance(m_data.begin(), it);
    auto tl = index(distance, 0);
    auto br = index(distance, 2);

    emit dataChanged(tl, br, { Qt::DisplayRole, Qt::BackgroundRole });
  }
}

//-----------------------------------------------------------------------------
void ObjectsTableModel::addObject(const QString &obj, const QColor &color)
{
  beginInsertRows(QModelIndex(), m_data.size(), m_data.size());

  m_data.emplace_back(obj.toStdWString(), std::wstring(), 0, color);

  endInsertRows();
}

//-----------------------------------------------------------------------------
QString ObjectsTableModel::eventText(const Events &e)
{
  switch(e)
  {
    case Events::ADDED:
      return tr("Added file");
      break;
    case Events::MODIFIED:
      return tr("Modified file");
      break;
    case Events::REMOVED:
      return tr("Removed file");
      break;
    case Events::RENAMED_OLD:
      // no break
    case Events::RENAMED_NEW:
      return tr("Renamed a file");
      break;
    default:
      break;
  }

  return tr("Unknown event");
}

//-----------------------------------------------------------------------------
void ObjectsTableModel::resetObject(const std::wstring &obj)
{
  auto findObject = [&obj](std::tuple<std::wstring, std::wstring, unsigned long, QColor> &p)
  {
    return std::get<0>(p).compare(obj) == 0;
  };
  auto it = std::find_if(m_data.begin(), m_data.end(), findObject);

  if(it != m_data.end())
  {
    auto &data = *it;
    std::get<1>(data) = std::wstring();
    std::get<2>(data) = 0;

    const auto distance = std::distance(m_data.begin(), it);
    auto tl = index(distance, 1);
    auto br = index(distance, 2);

    emit dataChanged(tl, br, { Qt::DisplayRole, Qt::BackgroundRole });
  }
}

//-----------------------------------------------------------------------------
void ObjectsTableModel::removeObject(const std::wstring &obj)
{
  auto findObject = [&obj](std::tuple<std::wstring, std::wstring, unsigned long, QColor> &p)
  {
    return std::get<0>(p).compare(obj) == 0;
  };
  auto it = std::find_if(m_data.begin(), m_data.end(), findObject);

  if(it != m_data.end())
  {
    const auto distance = std::distance(m_data.begin(), it);
    beginRemoveRows(QModelIndex(), distance, distance);
    m_data.erase(it);
    endRemoveRows();
  }
}
